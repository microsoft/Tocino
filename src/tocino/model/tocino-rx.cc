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

NS_LOG_COMPONENT_DEFINE ("TocinoRx");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->m_address.GetX() << "," \
                << (int) m_tnd->m_address.GetY() << "," \
                << (int) m_tnd->m_address.GetZ() << ") " \
                << m_portNumber << " "; }
#endif

namespace ns3 {

TocinoRx::TocinoRx( Ptr<TocinoNetDevice> tnd )
    : m_portNumber( TOCINO_INVALID_PORT )
    , m_xstate( TocinoFlowControl::XON )
    , m_tnd( tnd )
    , m_queues( tnd->GetNPorts() * tnd->GetNVCs() )
{}

TocinoRx::~TocinoRx()
{}
    
Ptr<NetDevice> TocinoRx::GetNetDevice()
{ 
    return m_tnd;
}

bool
TocinoRx::IsBlocked()
{
    uint32_t i;

    // Receiver is blocked if ANY queue is full
    for (i = 0; i < m_queues.size(); i++)
    {
        if (m_queues[i]->IsFull()) 
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

    if (m_xstate == TocinoFlowControl::XOFF)
    {
        // if not blocked, schedule XON
        if (!IsBlocked()) 
        {
            //NS_LOG_LOGIC("unblocked" );
            m_tnd->m_transmitters[m_portNumber]->SendXON();
            m_tnd->m_transmitters[m_portNumber]->Transmit(); // restart the transmitter
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
TocinoRx::Receive(Ptr<Packet> p)
{
    // FIXME: Use some kind of packet ID here instead
    NS_LOG_FUNCTION( PeekPointer(p) );

    uint32_t tx_q, tx_port;

    if (TocinoFlowControl::IsXONPacket(p)) // XON packet enables transmission on this port
    {
        NS_LOG_LOGIC("received XON");
        m_tnd->m_transmitters[m_portNumber]->SetXState(TocinoFlowControl::XON);
        m_tnd->m_transmitters[m_portNumber]->Transmit(); // restart the transmitter
        return;
    }
    
    if (TocinoFlowControl::IsXOFFPacket(p)) // XOFF packet disables transmission on this port
    {
        NS_LOG_LOGIC("received XOFF");
        m_tnd->m_transmitters[m_portNumber]->SetXState(TocinoFlowControl::XOFF);
        return;
    }
  
    Ptr<TocinoRouter> router = m_tnd->GetRouter(); //FIXME this should be done at init time
    NS_ASSERT( router != NULL );

    // figure out where the packet goes; returns linearized <port, vc> index
    tx_q = router->Route( m_portNumber, p ); 
    
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
            m_tnd->m_transmitters[m_portNumber]->SendXOFF();
            m_tnd->m_transmitters[m_portNumber]->Transmit();
        }
    }
    m_tnd->m_transmitters[tx_port]->Transmit(); // kick the transmitter
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
