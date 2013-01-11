/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>
#include <sstream>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"
#include "tocino-misc.h"
#include "tocino-router.h"
#include "tocino-dimension-order-router.h"
#include "tocino-flit-id-tag.h"

NS_LOG_COMPONENT_DEFINE ("TocinoRx");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->GetTocinoAddress().GetX() << "," \
                << (int) m_tnd->GetTocinoAddress().GetY() << "," \
                << (int) m_tnd->GetTocinoAddress().GetZ() << ") " \
                << m_portNumber << " "; }
#endif

namespace ns3 {

TocinoRx::TocinoRx( const uint32_t portNumber, Ptr<TocinoNetDevice> tnd, Ptr<TocinoRouter> router )
    : m_portNumber( portNumber )
    , m_tnd( tnd )
    , m_tx( tnd->GetTransmitter( portNumber ) )
    , m_router( router )
{
    m_queues.resize( m_tnd->GetNPorts() );
    for( uint32_t port = 0; port < m_queues.size(); ++port )
    {
        m_queues[port].resize( m_tnd->GetNVCs() );
        for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
        {
            m_queues[port][vc].resize( m_tnd->GetNVCs() );
        }
    }
}

TocinoRx::~TocinoRx()
{}

uint32_t
TocinoRx::GetPortNumber() const
{
    return m_portNumber;
}

Ptr<NetDevice>
TocinoRx::GetNetDevice()
{ 
    return m_tnd;
}

bool
TocinoRx::IsQueueBlocked(
        const uint32_t outputPort,
        const uint8_t inputVC,
        const uint8_t outputVC ) const
{
    NS_ASSERT( outputPort < m_tnd->GetNPorts() );
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );

    return m_queues[outputPort][inputVC][outputVC]->IsAlmostFull();
}

// ISSUE-REVIEW: Should this function exist at all?
bool
TocinoRx::IsAnyQueueBlocked() const
{
    for( uint32_t outputPort = 0; outputPort < m_tnd->GetNPorts(); ++outputPort )
    {
        for( uint8_t inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
        {
            for( uint8_t outputVC = 0; outputVC < m_tnd->GetNVCs(); ++outputVC )
            {
                if( IsQueueBlocked( outputPort, inputVC, outputVC ) )
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// ISSUE-REVIEW: the argument here MUST be an inputVC.
// It's not a meaningful question otherwise.  Perhaps
// we should create an InputVC type, so the compiler
// can help us avoid a mistake?
bool
TocinoRx::IsVCBlocked( const uint8_t inputVC ) const
{
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );

    for( uint32_t outputPort = 0; outputPort < m_tnd->GetNPorts(); ++outputPort )
    {
        for( uint8_t outputVC = 0; outputVC < m_tnd->GetNVCs(); ++outputVC )
        {
            if( IsQueueBlocked( outputPort, inputVC, outputVC ) )
            {
                return true;
            }
        }
    }
    return false;
}

void
TocinoRx::SetQueue(
        uint32_t outputPort,
        uint8_t inputVC,
        uint8_t outputVC,
        Ptr<CallbackQueue> q )
{
    NS_ASSERT( outputPort < m_tnd->GetNPorts() );
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );

    m_queues[outputPort][inputVC][outputVC] = q;
}

void
TocinoRx::RewriteFlitHeaderVC( Ptr<Packet> f, const uint8_t newVC ) const
{
    TocinoFlitHeader h;
    f->RemoveHeader( h );
    
    uint8_t currentVC = h.GetVirtualChannel();

    NS_ASSERT_MSG( newVC != currentVC, "Pointless rewrite?" );

    h.SetVirtualChannel( newVC );

    f->AddHeader( h );
}

bool
TocinoRx::EnqueueHelper( Ptr<Packet> flit, const TocinoQueueDescriptor qd )
{
    // N.B.
    // 
    // We are about to enqueue into m_queues[...].
    // Afterward, if the receiver is blocked, the
    // cause is unambiguous: it must be due to our
    // enqueue. Thus, it is tempting to avoid a loop
    // and full scan of all the queues by making the
    // post-enqueue check into:
    // 
    // isNowAlmostfull = m_queues[...]->IsAlmostFull()
    //
    // However, for symmetry (the "blocked" concept
    // seems better here than "almost full"), as well
    // as to avoid premature optimization, we will not
    // do this.
    //
    //  -MAS
    
    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( flit ) == qd.outputVC,
        "attempt to enqueue flit with mismatched VC" );
     
    bool wasNotBlocked = !IsVCBlocked( qd.inputVC );

    bool success
        = m_queues[qd.port][qd.inputVC][qd.outputVC]->Enqueue( flit );

    bool isNowBlocked = IsVCBlocked( qd.inputVC );
    
    NS_ASSERT_MSG( success, "queue overrun "
            << qd.port << ":"
            << (uint32_t) qd.inputVC << ":"
            << (uint32_t) qd.outputVC );
    
    bool enqueueTriggeredBlock = wasNotBlocked && isNowBlocked;

    return enqueueTriggeredBlock;
}

void
TocinoRx::Receive( Ptr<Packet> f )
{
    NS_LOG_FUNCTION( GetTocinoFlitIdString( f ) );
    
    if( IsTocinoFlowControlFlit( f ) )
    {
        NS_LOG_LOGIC( "got flow control flit" );
       
        TocinoFlowControlState newXState = GetTocinoFlowControlState( f );
        m_tx->SetXState( newXState );

        return;
    }
 
    NS_ASSERT( m_router != NULL );
    
    // figure out where the flit goes
    TocinoQueueDescriptor tx_q = m_router->Route( f ); 
    
    NS_ASSERT_MSG( tx_q != TOCINO_INVALID_QUEUE, "Route failed" );

    // Router may have elected to change virtual channels
    if( tx_q.inputVC != tx_q.outputVC )
    {
        RewriteFlitHeaderVC( f, tx_q.outputVC );
    }
   
    bool enqueueTriggeredBlock = EnqueueHelper( f, tx_q );

    if( enqueueTriggeredBlock )
    {
        // FIXME:
        // We intend to model an ejection port that can never be full. Yet,
        // we call RemotePause even when tx_port == m_tnd->GetHostPort(),
        // in order to avoid overrunning the ejection port queues.

        if( m_portNumber != m_tnd->GetHostPort() )
        {
            m_tx->RemotePause( tx_q.inputVC );
        }
    }

    // kick the transmitter
    m_tnd->GetTransmitter( tx_q.port )->Transmit();
}

void
TocinoRx::SetReserveFlits( uint32_t numFlits )
{
    for( uint32_t outputPort = 0; outputPort < m_tnd->GetNPorts(); ++outputPort )
    {
        for( uint8_t inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
        {
            for( uint8_t outputVC = 0; outputVC < m_tnd->GetNVCs(); ++outputVC )
            {
                m_queues[outputPort][inputVC][outputVC]->SetFreeWM( numFlits );
            }
        }
    }
}

void
TocinoRx::DumpState()
{
    NS_LOG_LOGIC("receiver=" << m_portNumber);
    for( uint32_t inVC = 0; inVC < m_tnd->GetNVCs(); ++inVC )
    {
        if( IsVCBlocked( inVC ) )
        {
            NS_LOG_LOGIC(" inVC=" << inVC << " BLOCKED");
        }
        else
        {
            NS_LOG_LOGIC(" inVC=" << inVC << " not blocked");
        }

        for( uint32_t outVC = 0; outVC < m_tnd->GetNVCs(); ++outVC )
        {
            TocinoQueueDescriptor qd = m_router->GetCurrentRoute( inVC );

            if( qd != TOCINO_INVALID_QUEUE )
            {
                if( (qd.inputVC == inVC) && (qd.outputVC == outVC) )
                {
                    NS_LOG_LOGIC("  route in-progress to outputPort=" << qd.port 
                            << " outVC=" << (uint32_t) qd.outputVC );
                }
            }

            for( uint32_t outputPort = 0; outputPort < m_tnd->GetNPorts(); ++outputPort )
            {
                Ptr<CallbackQueue> queue = m_queues[outputPort][inVC][outVC];
                
                if( queue->Size() > 0 )
                {
                    if( IsQueueBlocked( outputPort, inVC, outVC ) )
                    {
                        NS_LOG_LOGIC("  queue for outputPort=" 
                                << outputPort << " outVC=" << outVC << " BLOCKED" );
                    }
                    else
                    {
                        NS_LOG_LOGIC("  queue for outputPort=" 
                                << outputPort << " outVC=" << outVC << " not blocked" );
                    }

                    for( uint32_t i = 0; i < queue->Size(); i++ )
                    {
                        NS_LOG_LOGIC("   " << GetTocinoFlitIdString( queue->At(i) ) );
                    }
                }

            }
        }
    }
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
