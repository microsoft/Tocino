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

namespace ns3 {

TocinoRx::TocinoRx( Ptr<TocinoNetDevice> tnd )
    : m_portNumber( TOCINO_INVALID_PORT )
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
//            char str[64];
            //          sprintf(str, "blocked on %d(0x%08x)", i, (unsigned int)&m_queues[i]);
            //NS_LOG_LOGIC(str);
            return true;
        }
    }
    return false;
}

void
TocinoRx::CheckForUnblock()
{
    //NS_LOG_FUNCTION(this);
    //char str[64];

    if (m_tnd->m_transmitters[m_portNumber]->GetXState() == TocinoFlowControl::XOFF)
    {
        // if not blocked, schedule XON
        if (!IsBlocked()) 
        {
            //      sprintf(str, "%d unblocked", m_portNumber);
            //NS_LOG_LOGIC(str);
            m_tnd->m_transmitters[m_portNumber]->SendXON();
        }
        else
        {
            //sprintf(str, "%d remains blocked", m_portNumber);
            //NS_LOG_LOGIC(str);
        }
    }
    else
    {
        //NS_LOG_LOGIC("channel is not XOFF");
    }
}

void
//TocinoRx::Receive(Ptr<const Packet> p)
TocinoRx::Receive(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(m_tnd->GetNode()->GetId() << this->m_portNumber << PeekPointer(p));

    uint32_t tx_q, tx_port;

    // XON packet enables transmission on this port
    if (TocinoFlowControl::IsXONPacket(p))
    {
        //NS_LOG_LOGIC("setting pending XON");
        m_tnd->m_transmitters[m_portNumber]->SetXState(TocinoFlowControl::XON);
        m_tnd->m_transmitters[m_portNumber]->Transmit();
        return;
    }

    // XOFF packet disables transmission on this port
    if (TocinoFlowControl::IsXOFFPacket(p))
    {
        //NS_LOG_LOGIC("setting pending XOFF");
        m_tnd->m_transmitters[m_portNumber]->SetXState(TocinoFlowControl::XOFF);
        return;
    }
  
    Ptr<TocinoRouter> router = m_tnd->GetRouter();
    NS_ASSERT( router != NULL );

    // figure out where the packet goes
    // returns linearized <port, vc> index
    tx_q = router->Route( m_portNumber, p ); 
    
    NS_ASSERT_MSG( tx_q != TOCINO_INVALID_QUEUE, "Route failed" );
    
    tx_port = m_tnd->QueueToPort( tx_q ); // extract port number from q index
   
    bool success = m_queues[tx_q]->Enqueue(p);
    NS_ASSERT_MSG (success, "queue overrun");

    // if the buffer is full, send XOFF - XOFF blocks ALL traffic to the port
    if (m_queues[tx_q]->IsFull())
    {
        //NS_LOG_LOGIC("full buffer");

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
