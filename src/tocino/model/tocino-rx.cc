/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"

namespace ns3 {

TocinoRx::TocinoRx( uint32_t nPorts, uint32_t nVCs )
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
    uint32_t r;
    // return to vc 0 on injection port - Route returns buffer index
    r = (m_tnd->injectionPortNumber() * m_tnd->m_nVCs) + 0;
    return r;
}

} // namespace ns3
