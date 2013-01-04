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
    , m_queues( tnd->GetNQueues() )
    , m_router( router )
{}

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
TocinoRx::IsQueueBlocked( uint32_t qnum ) const
{
    return m_queues[qnum]->IsAlmostFull();
}

bool
TocinoRx::IsAnyQueueBlocked() const
{
    for( uint32_t i = 0; i < m_queues.size(); ++i )
    {
        if( IsQueueBlocked( i ) )
        {
            return true;
        }
    }
    return false;
}

bool
TocinoRx::IsVCBlocked( const uint8_t vc ) const
{
    const uint32_t BASE = vc;
    const uint32_t INCR = m_tnd->GetNVCs();
    
    for( uint32_t i = BASE; i < m_queues.size(); i += INCR )
    {
        if( IsQueueBlocked( i ) )
        {
            return true;
        }
    }
    return false;
}

void
TocinoRx::SetQueue( uint32_t qnum, Ptr<CallbackQueue> q )
{
    NS_ASSERT( qnum < m_queues.size() );
    m_queues[qnum] = q;
}

bool
TocinoRx::EnqueueHelper( Ptr<Packet> flit, uint32_t queue )
{
    // N.B.
    // 
    // We are about to enqueue into m_queues[queue].
    // Afterward, if the receiver is blocked, the
    // cause is unambiguous: it must be due to our
    // enqueue. Thus, it is tempting to avoid a loop
    // and full scan of all the queues by making the
    // post-enqueue check into:
    // 
    // isNowAlmostfull = m_queues[queue]->IsAlmostFull()
    //
    // However, for symmetry (the "blocked" concept
    // seems better here than "almost full"), as well
    // as to avoid premature optimization, we will not
    // do this.
    //
    //  -MAS
    
    uint8_t tx_vc = m_tnd->QueueToVC( queue );
    
    TocinoFlitHeader h;
    flit->PeekHeader( h );

    NS_ASSERT_MSG( h.GetVirtualChannel() == tx_vc,
        "attempt to enqueue flit with mismatched VC" );
     
    bool wasNotBlocked = !IsVCBlocked( tx_vc );
    bool success = m_queues[queue]->Enqueue( flit );
    bool isNowBlocked = IsVCBlocked( tx_vc );
    
    NS_ASSERT_MSG( success, "queue overrun " << queue );
    
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
    uint32_t tx_q = m_router->Route( f ); 
    
    NS_ASSERT_MSG( tx_q != TOCINO_INVALID_QUEUE, "Route failed" );
    
    uint32_t tx_port = m_tnd->QueueToPort( tx_q );
    uint8_t tx_vc = m_tnd->QueueToVC( tx_q );

    bool enqueueTriggeredBlock = EnqueueHelper( f, tx_q );

    if( enqueueTriggeredBlock )
    {
        // FIXME:
        // We intend to model an ejection port that can never be full. Yet,
        // we call RemotePause even when tx_port == m_tnd->GetHostPort(),
        // in order to avoid overrunning the ejection port queues.

        if( m_portNumber != m_tnd->GetHostPort() )
        {
            m_tx->RemotePause( tx_vc );
        }
    }

    // kick the transmitter
    m_tnd->GetTransmitter(tx_port)->Transmit();
}

void
TocinoRx::SetReserveFlits( uint32_t numFlits )
{
    for( uint32_t i = 0; i < m_queues.size(); ++i )
    {
        m_queues[i]->SetFreeWM( numFlits );
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
                uint32_t q = (port * m_tnd->GetNVCs()) + vc;
                if (IsQueueBlocked(q))
                {
                    NS_LOG_LOGIC("  blocked on port=" << port);
                    for (uint32_t i = 0; i < m_queues[q]->Size(); i++)
                    {
                        NS_LOG_LOGIC("   " << GetTocinoFlitIdString(m_queues[q]->At(i)));
                    }
                }
            }
        }
    }
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
