/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/data-rate.h"

#include "tocino-tx.h"
#include "tocino-rx.h"
#include "callback-queue.h"
#include "tocino-net-device.h"
#include "tocino-channel.h"

NS_LOG_COMPONENT_DEFINE ("TocinoTx");

namespace ns3 {

TocinoTx::TocinoTx(uint32_t nPorts, uint32_t nVCs)
{
  m_portNumber = 0xffffffff;
  m_xstate = TocinoFlowControl::XON;
  m_state = IDLE;
  m_pending_xon = false;
  m_pending_xoff = false;
  m_queues.resize(nPorts*nVCs);
}

TocinoTx::~TocinoTx()
{}

void TocinoTx::SetXState(TocinoFlowControl::State s)
{
    m_xstate = s;
}

TocinoFlowControl::State TocinoTx::GetXState()
{
    return m_xstate;
}

void TocinoTx::SetChannel(Ptr<TocinoChannel> channel)
{
    m_channel = channel;
}

void TocinoTx::SendXOFF()
{
    m_pending_xoff = true;
}

void TocinoTx::SendXON()
{
    m_pending_xon = true;
}

Ptr<NetDevice> TocinoTx::GetNetDevice()
{
    return m_tnd;
}

void
TocinoTx::TransmitEnd()
{
  m_state = IDLE;
  Transmit();
}

void
TocinoTx::Transmit()
{
    Time transmit_time;
    Ptr<const Packet> p = 0;
    uint32_t winner, rx_port;
    
    if (m_state == BUSY) return;
    
    NS_ASSERT_MSG(!(m_pending_xoff && m_pending_xon), "race condition detected");
    
    // send an XOFF is one is pending
    if (m_pending_xoff)
    {
        m_pending_xoff = false;
        if (m_xstate == TocinoFlowControl::XON) // only send if we're currently enabled
	{
            if (m_portNumber == m_tnd->injectionPortNumber())
            {
                // kick the net device
                m_tnd->InjectFlits();
            }
            else
            {
                NS_LOG_LOGIC ("scheduling XOFF");
                p = TocinoFlowControl::GetXOFFPacket();
            }
	}
    }
    
    if (!p && m_pending_xon)
    {
        m_pending_xon = false;
        if (m_xstate == TocinoFlowControl::XOFF) // only send if we're currently disabled
	{
            NS_LOG_LOGIC ("scheduling XON");
            p = TocinoFlowControl::GetXONPacket();
	}
    }
    
    if (!p && (m_xstate == TocinoFlowControl::XON)) // legal to transmit
    {
        winner = Arbitrate();
        if (winner < m_queues.size())
        {
            rx_port = winner/m_tnd->m_nVCs;

            // if we've unblocked the winner receive port we need to cause an XON
            // to be scheduled on its corresponding transmit port (hide the crud in
            // TocinoNetDeviceReceiver::CheckForUnblock())
            //
            // check for full must occur before CheckForUnblock but Dequeue must occur
            // whether the queue was full or not
            if (m_queues[rx_port]->IsFull())
            {
                p = m_queues[winner]->Dequeue();
                m_tnd->m_receivers[rx_port]->CheckForUnblock();
            }
            else
            {
                p = m_queues[winner]->Dequeue();
            }
        
            NS_ASSERT_MSG( p != NULL, "Queue underrun?" );
        }
    }

    if (p) // send the packet onward
    {
        if (m_portNumber == m_tnd->injectionPortNumber()) // connected to ejection port
        {
            // eject packet
            m_tnd->EjectFlit(p);
        }
        else
        {
            NS_ASSERT_MSG( m_channel != NULL, "Trying to send on a NULL channel" );

            // send packet to channel
            m_state = BUSY;
            m_channel->TransmitStart(p);
            transmit_time= m_channel->GetTransmissionTime(p);
            Simulator::Schedule(transmit_time, &TocinoTx::TransmitEnd, this);
        }
    }
}

uint32_t
TocinoTx::Arbitrate()
{
  uint32_t i;

    // trivial arbitration - obvious starvation concern
    for (i = 0; i < m_queues.size(); i++)
    {
        if (m_queues[i]->IsEmpty() == false) return i;
    }
    return m_queues.size(); // nothing pending
}

} // namespace ns3
