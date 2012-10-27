/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"

NS_LOG_COMPONENT_DEFINE ("TocinoRx");

namespace ns3 {

TocinoRx::TocinoRx( uint32_t nPorts, uint32_t nVCs )
    : m_portNumber( INVALID_PORT )
    , m_currentRoutePort( INVALID_PORT )
{
    m_queues.resize(nPorts*nVCs);
}

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

    // figure out where the packet goes
    tx_q = Route(p); // return linearized <port, vc> index
    tx_port = tx_q/m_tnd->m_nVCs; // extract port number from q index
    
    bool success = m_queues[tx_q]->Enqueue(p);
    NS_ASSERT_MSG (success, "queue overrun");

    // if the buffer is full, send XOFF - XOFF blocks ALL traffic to the port
    if (m_queues[tx_q]->IsFull())
    {
        //NS_LOG_LOGIC("full buffer");

        // ejection port should never be full
        NS_ASSERT_MSG(tx_port != m_tnd->ejectionPortNumber(), "ejection port is full");

        m_tnd->m_transmitters[m_portNumber]->SendXOFF();
        m_tnd->m_transmitters[m_portNumber]->Transmit();
    }
    m_tnd->m_transmitters[tx_port]->Transmit(); // kick the transmitter
}

uint32_t
TocinoRx::Route(Ptr<const Packet> p)
{
    TocinoFlitHeader h;
    p->PeekHeader( h );

    int outputPort = m_currentRoutePort; //FIXME need entry for each VC

    //NS_LOG_FUNCTION(PeekPointer(p) << this->m_currentRoutePort);

    if( h.IsHead() )
    {
        NS_ASSERT( m_currentRoutePort == INVALID_PORT );

        uint8_t x = m_tnd->m_address.GetX();
        uint8_t dx = h.GetDestination().GetX();

        uint8_t y = m_tnd->m_address.GetY();
        uint8_t dy = h.GetDestination().GetY();

        uint8_t z = m_tnd->m_address.GetZ();
        uint8_t dz = h.GetDestination().GetZ();

        // dimension-order routing
        
        if( dx > x ) 
        {
            //  NS_LOG_LOGIC("routing to 0/x+");
            outputPort = 0; // x+
        }
        else if( dx < x )
        {
            //NS_LOG_LOGIC("routing to 1/x-");
            outputPort = 1; // x-
        }
        else if( dy > y )
        {
            //NS_LOG_LOGIC("routing to 2/y+");
            outputPort = 2; // y+
        }
        else if( dy < y )
        {
            //NS_LOG_LOGIC("routing to 3/y-");
            outputPort = 3; // y-
        }
        else if( dz > z )
        {
            //NS_LOG_LOGIC("routing to 4/z+");
            outputPort = 4; // z+
        }
        else if( dz < z )
        {
            //NS_LOG_LOGIC("routing to 5/z-");
            outputPort = 5; // z-
        }
        else
        {
            NS_ASSERT( h.GetDestination() == m_tnd->GetAddress() ); // deliver flit to host
            //NS_LOG_LOGIC("routing to 6/ejection");
            outputPort = m_tnd->ejectionPortNumber(); 
        }

        m_currentRoutePort = outputPort;
    }
    else
    {
        //char str[64];
        //sprintf(str, "routing on established path to %d", outputPort);
        //NS_LOG_LOGIC(str);
    }
    NS_ASSERT( m_currentRoutePort != INVALID_PORT );

    if( h.IsTail() )
    {
        m_currentRoutePort = INVALID_PORT;
        //NS_LOG_LOGIC("removing established path");
    }

    return (outputPort * m_tnd->m_nVCs); //FIXME always to vc 0 for now
}

} // namespace ns3
