/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-net-device.h"

namespace ns3 {

NS_OBJECT_ENSURE_DEFINED (TocinoNetDeviceTransmitter);

TocinoNetDeviceTransmitter::TocinoNetDeviceTransmitter(Ptr<TocinoNetDevice nd,
						       uint32_t port,
						       uint32_t nPorts)
{
  m_nd = nd;
  m_port = port;
  m_nPorts = nPorts; // m_nPorts includes the injection/ejection port!!!
}

void
TocinoNetDeviceTransmitter::DefinePortLinkage(uint32_t port, 
					Ptr<TocinoNetDeviceReceiver> receiver, 
					Ptr<TocinoQueue> q)
{
  m_receivers[port] = receiver;
  m_queues[port] = q;
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
  Time t;
  Ptr<Packet> p = NULL;
  uint32_t winner;

  if (m_state == BUSY) return;

  if (m_pending_xoff && m_pending_xon)
    {
      // this is a race and probably an error
      cerr << "race on port " << m_port << "; exiting";
      exit(1);
    }

  // send an XOFF is one is pending
  if (m_pending_xoff)
    {
      m_pending_xoff = false;
      if (m_xstate == XON) // only send if we're currently enabled
	{
	  p = xoff_packet.Copy();
	}
    }
  else if (m_pending_xon)
    {
      m_pending_xon = false;
      if (m_xstate == XOFF) // only send if we're currently disabled
	{
	  p = xon_packet.Copy();
	}
    }
  else if (m_xstate == XON) // legal to transmit
    {
      winner = Arbitrate();
      if (winner == m_nPorts)
        {
          // queues are empty
        }
      else
        {
          // if we've unblocked the winner receive port we need to cause an XON
          // to be scheduled on its corresponding transmit port (hide the crud in
          // TocinoNetDeviceReceiver)
          if (m_queues[winner]->IsFull())
            {
              p = m_queues[winner]->Dequeue();
              m_receivers[winner]->CheckForUnblock();
            }
        }
    }
  if (p != NULL)
    {
      m_state = BUSY;
      m_channel->TransmitStart(p);
      t = (p->SizeInBytes() * 8) / m_channel->GetDataRate();
      Simulation::Schedule(t, TransmitEnd); // will this invoke the correct context???
      return;
    }
}

uint32_t
TocinoNetDeviceTransmitter::Arbitrate
{
  uint32_t i;

  // trivial arbitration - obvious starvation concern
  for (i = 0; i < m_nPorts; i++)
    {
      if (m_queues[i].IsEmpty() == false) return i;
    }
  return m_nPorts;
}

} // namespace ns3

