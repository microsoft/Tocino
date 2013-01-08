/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"
#include "tocino-misc.h"
#include "tocino-router.h"
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
TocinoRx::IsQueueBlocked( const uint32_t port, const uint8_t vc ) const
{
    NS_ASSERT( port < m_tnd->GetNPorts() );
    NS_ASSERT( vc < m_tnd->GetNVCs() );

    return m_queues[port][vc]->IsAlmostFull();
}

// ISSUE-REVIEW: Should this function exist at all?
bool
TocinoRx::IsAnyQueueBlocked() const
{
    for( uint32_t port = 0; port < m_tnd->GetNPorts(); ++port )
    {
        for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
        {
            if( IsQueueBlocked( port, vc ) )
            {
                return true;
            }
        }
    }
    return false;
}

bool
TocinoRx::IsVCBlocked( const uint8_t vc ) const
{
    NS_ASSERT( vc < m_tnd->GetNVCs() );

    for( uint32_t port = 0; port < m_tnd->GetNPorts(); ++port )
    {
        if( IsQueueBlocked( port, vc ) )
        {
            return true;
        }
    }
    return false;
}

void
TocinoRx::SetQueue( uint32_t port, uint8_t vc, Ptr<CallbackQueue> q )
{
    NS_ASSERT( port < m_tnd->GetNPorts() );
    NS_ASSERT( vc < m_tnd->GetNVCs() );

    m_queues[port][vc] = q;
}

bool
TocinoRx::EnqueueHelper( Ptr<Packet> flit, const uint32_t port, const uint8_t vc )
{
    // N.B.
    // 
    // We are about to enqueue into m_queues[port][vc].
    // Afterward, if the receiver is blocked, the
    // cause is unambiguous: it must be due to our
    // enqueue. Thus, it is tempting to avoid a loop
    // and full scan of all the queues by making the
    // post-enqueue check into:
    // 
    // isNowAlmostfull = m_queues[queue][vc]->IsAlmostFull()
    //
    // However, for symmetry (the "blocked" concept
    // seems better here than "almost full"), as well
    // as to avoid premature optimization, we will not
    // do this.
    //
    //  -MAS
    
    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( flit ) == vc,
        "attempt to enqueue flit with mismatched VC" );
     
    bool wasNotBlocked = !IsVCBlocked( vc );
    bool success = m_queues[port][vc]->Enqueue( flit );
    bool isNowBlocked = IsVCBlocked( vc );
    
    NS_ASSERT_MSG( success, "queue overrun " << port << ":" << vc );
    
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
 
    // figure out where the flit goes; returns linearized <port, vc> index
    NS_ASSERT( m_router != NULL );
    TocinoQueueDescriptor tx_q = m_router->Route( f ); 
    
    NS_ASSERT_MSG( tx_q != TOCINO_INVALID_QUEUE, "Route failed" );
    
    bool enqueueTriggeredBlock = EnqueueHelper( f, tx_q.port, tx_q.vc );

    if( enqueueTriggeredBlock )
    {
        // FIXME:
        // We intend to model an ejection port that can never be full. Yet,
        // we call RemotePause even when tx_port == m_tnd->GetHostPort(),
        // in order to avoid overrunning the ejection port queues.

        if( m_portNumber != m_tnd->GetHostPort() )
        {
            m_tx->RemotePause( tx_q.vc );
        }
    }

    // kick the transmitter
    m_tnd->GetTransmitter(tx_q.port)->Transmit();
}

void
TocinoRx::SetReserveFlits( uint32_t numFlits )
{
    for( uint32_t port = 0; port < m_tnd->GetNPorts(); ++port )
    {
        for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
        {
            m_queues[port][vc]->SetFreeWM( numFlits );
        }
    }
}

void
TocinoRx::DumpState()
{
    NS_LOG_LOGIC("receiver=" << m_portNumber);
    for (uint32_t vc = 0; vc < 1; vc++) // change loop bound to enable additional VCs
    {
        if (IsVCBlocked(vc))
        {
            NS_LOG_LOGIC(" vc=" << vc << " BLOCKED");
            for (uint32_t port = 0; port < m_tnd->GetNPorts(); port++)
            {
                if (IsQueueBlocked(port, vc))
                {
                    NS_LOG_LOGIC("  blocked on port=" << port);
                    for (uint32_t i = 0; i < m_queues[port][vc]->Size(); i++)
                    {
                        NS_LOG_LOGIC("   " << GetTocinoFlitIdString(m_queues[port][vc]->At(i)));
                    }
                }
            }
        }
    }
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
