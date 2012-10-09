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

TocinoTx::TocinoTx(uint32_t nPorts)
{
  m_portNumber = 0xffffffff;
  m_xstate = XON;
  m_state = IDLE;
  m_pending_xon = false;
  m_pending_xoff = false;
  m_queues.resize(nPorts);
}

TocinoTx::~TocinoTx()
{}

void TocinoTx::SetXState(TocinoFlowControlState s)
{
    m_xstate = s;
}

TocinoFlowControlState TocinoTx::GetXState()
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
    Ptr<Packet> p = 0;
    uint32_t winner;
    
    if (m_state == BUSY) return;
    
    if (m_pending_xoff && m_pending_xon)
    {
        // this is a race and probably an error
        NS_ASSERT_MSG (false, "race condition detected");
        return;
    }
    
    // send an XOFF is one is pending
    if (m_pending_xoff)
    {
        m_pending_xoff = false;
        if (m_xstate == XON) // only send if we're currently enabled
	{
            if (m_portNumber == m_tnd->injectionPortNumber())
            {
                // trigger callback
                NS_LOG_LOGIC ("triggering injection port callback");
                m_tnd->m_injectionPortCallback();
            }
            else
            {
                NS_LOG_LOGIC ("scheduling XOFF");
                //p = xoff_packet.Copy();
            }
	}
    }
    
    if (!p && m_pending_xon)
    {
        m_pending_xon = false;
        if (m_xstate == XOFF) // only send if we're currently disabled
	{
            NS_LOG_LOGIC ("scheduling XON");
            //p = xon_packet.Copy();
	}
    }
    
    if (!p && (m_xstate == XON)) // legal to transmit
    {
        winner = Arbitrate();
        NS_ASSERT_MSG(winner < m_tnd->m_nPorts, "invalid winner");

        // if we've unblocked the winner receive port we need to cause an XON
        // to be scheduled on its corresponding transmit port (hide the crud in
        // TocinoNetDeviceReceiver::CheckForUnblock())
        //
        // check for full must occur before CheckForUnblock but Dequeue must occur
        // whether the queue was full or not
        if (m_queues[winner]->IsFull())
        {
            p = m_queues[winner]->Dequeue();
            m_tnd->m_receivers[winner]->CheckForUnblock();
        }
        else
        {
            p = m_queues[winner]->Dequeue();
        }
    }

    if (p) // send the packet onward
    {
        if (m_portNumber == m_tnd->injectionPortNumber()) // connected to ejection port
        {
            // eject packet - should never fail
            if (m_tnd->EjectFlit(p) == false)
            {
                NS_ASSERT_MSG(false, "ejection failed");
            }
        }
        else
        {
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
  for (i = 0; i < m_tnd->m_nPorts; i++)
    {
      if (m_queues[i]->IsEmpty() == false) return i;
    }
  return m_tnd->m_nPorts; // nothing pending
}

} // namespace ns3

