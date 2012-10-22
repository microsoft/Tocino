/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"

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
        if (m_queues[i]->IsFull()) return true;
    }
    return false;
}

void
TocinoRx::CheckForUnblock()
{

    if (m_tnd->m_transmitters[m_portNumber]->GetXState() == XOFF)
    {
        // if not blocked, schedule XON
        if (!IsBlocked()) m_tnd->m_transmitters[m_portNumber]->SendXON();
    }
}

void
TocinoRx::Receive(Ptr<Packet> p)
{
    uint32_t tx_q, tx_port;

    // XON packet enables transmission on this port
    if (/*p->IsXON()*/ 0)
    {
        m_tnd->m_transmitters[m_portNumber]->SetXState(XON);
        m_tnd->m_transmitters[m_portNumber]->Transmit();
        return;
    }

    // XOFF packet disables transmission on this port
    if (/*p->IsXOFF()*/ 0)
    {
        m_tnd->m_transmitters[m_portNumber]->SetXState(XOFF);
        return;
    }

    // figure out where the packet goes
    tx_q = Route(p); // return linearized <port, vc> index
    tx_port = tx_q/m_tnd->m_nVCs; // extract port number from q index
    m_queues[tx_q]->Enqueue(p);
    
    // if the buffer is full, send XOFF - XOFF blocks ALL traffic to the port
    if (m_queues[tx_q]->IsFull())
    {
        // ejection port should never be full
        NS_ASSERT_MSG(tx_port != m_tnd->injectionPortNumber(), "ejection port is full");

        m_tnd->m_transmitters[m_portNumber]->SendXOFF();
        m_tnd->m_transmitters[m_portNumber]->Transmit();
    }
    m_tnd->m_transmitters[tx_port]->Transmit(); // kick the transmitter
}

uint32_t
TocinoRx::Route(Ptr<Packet> p)
{
    TocinoFlitHeader h;
    p->PeekHeader( h );

    int outputPort = m_currentRoutePort;

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
            outputPort = 0; // x+
        }
        else if( dx < x )
        {
            outputPort = 1; // x-
        }
        else if( dy > y )
        {
            outputPort = 2; // y+
        }
        else if( dy < y )
        {
            outputPort = 3; // y-
        }
        else if( dz > z )
        {
            outputPort = 4; // z+
        }
        else if( dz < z )
        {
            outputPort = 5; // z-
        }
        else
        {
            // deliver successfully-routed flit to host
            NS_ASSERT( h.GetDestination() == m_tnd->GetAddress() );
            outputPort = m_tnd->injectionPortNumber(); 
        }

        m_currentRoutePort = outputPort;
    }
    
    NS_ASSERT( m_currentRoutePort != INVALID_PORT );

    if( h.IsTail() )
    {
        m_currentRoutePort = INVALID_PORT;
    }

    return (outputPort * m_tnd->m_nVCs); // vc 0
}

} // namespace ns3
