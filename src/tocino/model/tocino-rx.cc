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
    , m_upstreamXState( TocinoFlowControl::XON )
    , m_tnd( tnd )
    , m_queues( tnd->GetNQueues() )
    , m_router( router )
{}

TocinoRx::~TocinoRx()
{}
    
Ptr<NetDevice>
TocinoRx::GetNetDevice()
{ 
    return m_tnd;
}

void
TocinoRx::SetUpstreamXState( TocinoFlowControl::State s )
{
    m_upstreamXState = s;
}

TocinoFlowControl::State
TocinoRx::GetUpstreamXState() const
{
    return m_upstreamXState;
}
    
bool
TocinoRx::IsQueueBlocked( uint32_t qnum ) const
{
    return m_queues[qnum]->IsFull();
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
    const uint32_t INCR = vc+1;
    
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
TocinoRx::CheckForUnblock()
{
    //NS_LOG_FUNCTION(m_tnd->GetNode()->GetId() << m_portNumber);

    if (m_upstreamXState == TocinoFlowControl::XOFF)
    {
        // if not blocked, schedule XON
        if (!IsAnyQueueBlocked()) 
        {
            //NS_LOG_LOGIC("unblocked" );
            m_tnd->GetTransmitter(m_portNumber)->SendXON();
            m_tnd->GetTransmitter(m_portNumber)->Transmit(); // restart the transmitter
        }
        else
        {
            //NS_LOG_LOGIC("blocked" );
        }
    }
    else
    {
        //NS_LOG_LOGIC("is not XOFF" );
    }
}

void
TocinoRx::SetQueue( uint32_t qnum, Ptr<CallbackQueue> q )
{
    NS_ASSERT( qnum < m_queues.size() );
    m_queues[qnum] = q;
}

void
TocinoRx::Receive(Ptr<Packet> p)
{
    // FIXME: Use some kind of packet ID here instead
    NS_LOG_FUNCTION( PeekPointer(p) );

    uint32_t tx_q, tx_port;

    if (TocinoFlowControl::IsXONPacket(p)) // XON packet enables transmission on this port
    {
        NS_LOG_LOGIC("received XON");
        m_tnd->GetTransmitter(m_portNumber)->SetXState(TocinoFlowControl::XON);
        m_tnd->GetTransmitter(m_portNumber)->Transmit(); // restart the transmitter
        return;
    }
    
    if (TocinoFlowControl::IsXOFFPacket(p)) // XOFF packet disables transmission on this port
    {
        NS_LOG_LOGIC("received XOFF");
        m_tnd->GetTransmitter(m_portNumber)->SetXState(TocinoFlowControl::XOFF);
        return;
    }
  
    // figure out where the packet goes; returns linearized <port, vc> index
    NS_ASSERT( m_router != NULL );
    tx_q = m_router->Route( p ); 
    
    NS_ASSERT_MSG( tx_q != TOCINO_INVALID_QUEUE, "Route failed" );
    
    tx_port = m_tnd->QueueToPort( tx_q ); // extract port number from q index
   
    bool success = m_queues[tx_q]->Enqueue(p);
    NS_ASSERT_MSG (success, "queue overrun");

    // if the buffer is full, send XOFF - XOFF blocks ALL traffic to the port
    if (m_queues[tx_q]->IsFull())
    {
        if( tx_port == m_tnd->GetHostPort() )
        {
            // ejection port can never be full?
        }
        else
        {
            m_tnd->GetTransmitter(m_portNumber)->SendXOFF();
            m_tnd->GetTransmitter(m_portNumber)->Transmit();
        }
    }
    m_tnd->GetTransmitter(tx_port)->Transmit(); // kick the transmitter
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
