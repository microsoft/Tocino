/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/data-rate.h"

#include "callback-queue.h"
#include "tocino-channel.h"
#include "tocino-net-device.h"
#include "tocino-net-device-receiver.h"
#include "tocino-net-device-transmitter.h"

NS_LOG_COMPONENT_DEFINE ("TocinoNetDeviceTransmitter");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TocinoNetDeviceTransmitter);

TypeId TocinoNetDeviceTransmitter::GetTypeId(void)
{
  static TypeId tid = TypeId( "ns3::TocinoNetDeviceTransmitter" )
    .SetParent<Object>()
    .AddConstructor<TocinoNetDeviceTransmitter>()
    ;
  return tid;
}

TocinoNetDeviceTransmitter::TocinoNetDeviceTransmitter()
{
  m_channelNumber = 0xffffffff;
  m_xstate = TocinoNetDevice::XON;
  m_state = IDLE;
  m_pending_xon = false;
  m_pending_xoff = false;
}

void
TocinoNetDeviceTransmitter::TransmitEnd()
{
  m_state = IDLE;
  Transmit();
}

void
TocinoNetDeviceTransmitter::Transmit()
{
  Time transmit_time;
  Ptr<Packet> p = 0;
  uint32_t winner;

  if (m_state == BUSY) return;

  if (m_pending_xoff && m_pending_xon)
    {
      // this is a race and probably an error
      NS_LOG_ERROR ("Race condition detected");
      return;
    }

  // send an XOFF is one is pending
  if (m_pending_xoff)
    {
      m_pending_xoff = false;
      if (m_xstate == TocinoNetDevice::XON) // only send if we're currently enabled
	{
          NS_LOG_LOGIC ("Scheduling XON");
	  //p = xoff_packet.Copy();
	}
    }
  else if (m_pending_xon)
    {
      m_pending_xon = false;
      if (m_xstate == TocinoNetDevice::XOFF) // only send if we're currently disabled
	{
          NS_LOG_LOGIC ("Scheduling XOFF");
	  //p = xon_packet.Copy();
	}
    }
  else if (m_xstate == TocinoNetDevice::XON) // legal to transmit
    {
      winner = Arbitrate();
      if (winner == m_tnd->m_nPorts) // invalid winner -> nothing in queues
        {
          // queues are empty
        }
      else
        {
          // if we've unblocked the winner receive port we need to cause an XON
          // to be scheduled on its corresponding transmit port (hide the crud in
          // TocinoNetDeviceReceiver::CheckForUnblock())
          if (m_queues[winner]->IsFull())
            {
              p = m_queues[winner]->Dequeue();
              m_tnd->m_receivers[winner]->CheckForUnblock();
            }
        }
    }
  if (p) // send the packet onward
    {
      if (m_channelNumber == (m_tnd->m_nPorts-1)) // connected to ejection port
        {
          // eject packet
          if (m_tnd->EjectPacket(p) == false)
            {
              NS_LOG_ERROR ("Ejection failed");
            }
        }
      else
        {
          // send packet to channel
          m_state = BUSY;
          m_channel->TransmitStart(p);
          transmit_time= m_channel->GetTransmissionTime(p);
          Simulator::Schedule(transmit_time, &TocinoNetDeviceTransmitter::TransmitEnd, this);
        }
    }
}

uint32_t
TocinoNetDeviceTransmitter::Arbitrate()
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

