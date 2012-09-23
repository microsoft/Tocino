/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-net-device.h"

namespace ns3 {

class TocinoNetDeviceTransmitter;
class TocinoNetDeviceReceiver;
class TocinoQueue;

TocinoNetDeviceReceiver::TocinoNetDeviceReceiver(Ptr<TocinoNetDevice> nd,
						 uint32_t port,
						 uint32_t nPorts)
{
  m_nd = nd;
  m_port = port;
  m_nPorts = nPorts;
}

void
TocinoNetDeviceReceiver::DefinePortLinkage(uint32_t port, 
                                           Ptr<TocinoNetDeviceTransmitter> transmitter, 
                                           Ptr<TocinoQueue> q)
{
  m_transmitters[port] = transmitter;
  m_queues[port] = q;
}

void
TocinoNetDeviceReceiver::CheckForUnblock()
{
  if (m_transmitters[port]->GetXState() == XOFF)
    {
      // if NO queues are full, schedule XON
      for (i = 0; i < m_nPorts; i++)
	{
	  if (m_queues[i]->IsFull()) return;
	}
      m_transmitters[port]->SendXON();
    }
}

void
TocinoNetDeviceReceiver::Receive(Ptr<Packet> p)
{
  uint32_t tx_port;

  // XON packet enables transmission on this port
  if (p->IsXON())
    {
      m_transmitters[port]->SetXState(XON);
      m_transmitters[port]->Transmit();
      return;
    }

  // XOFF packet disables transmission on this port
  if (p->IsXOFF())
    {
      m_transmitters[port]->SetXState(XOFF);
      return;
    }

  // figure out where the packet goes and put it in the transmission queue
  tx_port = Route(p);
  m_queues[tx_port]->Enqueue(p);

  // if the buffer is full, send XOFF - blocks ALL traffic on this port
  if (m_queues[tx_port]->IsFull())
    {
      m_transmitters[m_port]->SendXOFF();
      m_transmitters[m_port]->Transmit();
    }
  m_transmitters[tx_port]->Transmit();
}

uint32_t
Route(Ptr<Packet> p)
{
  return 6; // implement loopback
}

} // namespace ns3
