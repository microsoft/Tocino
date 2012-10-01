/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "callback-queue.h"
#include "tocino-net-device.h"
#include "tocino-net-device-transmitter.h"
#include "tocino-net-device-receiver.h"

namespace ns3 {

class TocinoNetDevice;
class TocinoNetDeviceTransmitter;
class CallbackQueue;

bool
TocinoNetDeviceReceiver::IsBlocked()
{
  uint32_t i;

  // Receiver is blocked if ANY queue is full
  for (i = 0; i < m_tnd->m_nPorts; i++)
    {
      if (m_queues[i]->IsFull()) return true;
    }
  return false;
}

void
TocinoNetDeviceReceiver::CheckForUnblock()
{

  if (m_tnd->m_transmitters[m_channelNumber]->GetXState() == XOFF)
    {
      // if not blocked, schedule XON
      if (!IsBlocked()) m_tnd->m_transmitters[m_channelNumber]->SendXON();
    }
}

void
TocinoNetDeviceReceiver::Receive(Ptr<Packet> p)
{
  uint32_t tx_port;

  // XON packet enables transmission on this port
  if (/*p->IsXON()*/ 0)
    {
      m_tnd->m_transmitters[m_channelNumber]->SetXState(XON);
      m_tnd->m_transmitters[m_channelNumber]->Transmit();
      return;
    }

  // XOFF packet disables transmission on this port
  if (/*p->IsXOFF()*/ 0)
    {
      m_tnd->m_transmitters[m_channelNumber]->SetXState(XOFF);
      return;
    }

  // figure out where the packet goes
  tx_port = Route(p);
  m_queues[tx_port]->Enqueue(p);

  // if the buffer is full, send XOFF - XOFF blocks ALL traffic on the port
  if (m_queues[tx_port]->IsFull())
    {
      m_tnd->m_transmitters[m_channelNumber]->SendXOFF();
      m_tnd->m_transmitters[m_channelNumber]->Transmit();
    }
  m_tnd->m_transmitters[tx_port]->Transmit(); // kick the transmitter
}

uint32_t
TocinoNetDeviceReceiver::Route(Ptr<Packet> p)
{
  return m_tnd->m_nPorts-1; // loopback - send to ejection port
}

} // namespace ns3
