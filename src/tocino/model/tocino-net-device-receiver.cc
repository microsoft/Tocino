/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-queue.h"
#include "tocino-net-device.h"
#include "tocino-net-device-transmitter.h"
#include "tocino-net-device-receiver.h"

namespace ns3 {

class TocinoNetDevice;
class TocinoNetDeviceTransmitter;
class TocinoQueue;

TocinoNetDeviceReceiver::TocinoNetDeviceReceiver()
{
}

void
TocinoNetDeviceReceiver::CheckForUnblock()
{
  uint32_t i;

  if (m_transmitters[m_channelNumber]->GetXState() == TocinoNetDevice::XOFF)
    {
      // if NO queues are full, schedule XON
      for (i = 0; i < m_tnd->m_nPorts; i++)
	{
	  if (m_queues[i]->IsFull()) return;
	}
      m_transmitters[m_channelNumber]->SendXON();
    }
}

void
TocinoNetDeviceReceiver::Receive(Ptr<Packet> p)
{
  uint32_t tx_port;

  // XON packet enables transmission on this port
  if (/*p->IsXON()*/ 0)
    {
      m_transmitters[m_channelNumber]->SetXState(TocinoNetDevice::XON);
      m_transmitters[m_channelNumber]->Transmit();
      return;
    }

  // XOFF packet disables transmission on this port
  if (/*p->IsXOFF()*/ 0)
    {
      m_transmitters[m_channelNumber]->SetXState(TocinoNetDevice::XOFF);
      return;
    }

  // figure out where the packet goes and put it in the transmission queue
  tx_port = Route(p);
  m_queues[tx_port]->Enqueue(p);

  // if the buffer is full, send XOFF - blocks ALL traffic on this port
  if (m_queues[tx_port]->IsFull())
    {
      m_transmitters[m_channelNumber]->SendXOFF();
      m_transmitters[m_channelNumber]->Transmit();
    }
  m_transmitters[tx_port]->Transmit();
}

uint32_t
TocinoNetDeviceReceiver::Route(Ptr<Packet> p)
{
  return m_tnd->m_nPorts-1; // implement loopback
}

} // namespace ns3
