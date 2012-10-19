/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"

namespace ns3 {

TocinoRx::TocinoRx( uint32_t nPorts )
{
    m_queues.resize(nPorts);
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
    uint32_t tx_port;

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
    tx_port = Route(p);
    m_queues[tx_port]->Enqueue(p);
    
    // if the buffer is full, send XOFF - XOFF blocks ALL traffic to the port
    if (m_queues[tx_port]->IsFull())
    {
        // ejection port should never be full
        NS_ASSERT_MSG(tx_port != m_tnd->injectionPortNumber(), "ejection port is full");

        m_tnd->m_transmitters[m_portNumber]->SendXOFF();
        m_tnd->m_transmitters[m_portNumber]->Transmit();
    }
    m_tnd->m_transmitters[tx_port]->Transmit(); // kick the transmitter
}

namespace
{
    // silly hack for now
    const int INVALID = -1;
    int currentPort = INVALID;
}

uint32_t
TocinoRx::Route(Ptr<Packet> p)
{
    TocinoFlitHeader h;
    p->PeekHeader( h );

    int outputPort = currentPort;

    if( h.IsHead() )
    {
        NS_ASSERT( currentPort == INVALID );

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

        currentPort = outputPort;
    }
    
    NS_ASSERT( currentPort != INVALID );

    if( h.IsTail() )
    {
        currentPort = INVALID;
    }

    return outputPort;
}

} // namespace ns3
