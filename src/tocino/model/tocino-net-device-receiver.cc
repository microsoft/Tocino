/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-net-device.h"

namespace ns3 {

TocinoNetDeviceReceiver::TocinoNetDeviceReceiver(Ptr<TocinoNetDevice> nd,
						 unsigned int port,
						 unsigned int n_ports)
{
  m_nd = nd;
  m_port = port;
  m_n_ports = n_ports;
}

void
TocinoNetDeviceReceiver::PortLinkage(unsigned int port, 
				     Ptr<TocinoNetDeviceTransmitter> transmitter, 
				     Ptr<TocinoQueue> q)
{
  m_transmitter[port] = transmitter;
  m_q[port] = q;
}

void
TocinoNetDeviceReceiver::CheckForUnblock()
{
  if (m_transmitter[port]->GetXState() == XOFF)
    {
      // if NO queues are full, schedule XON
      for (i = 0; i < n_ports; i++)
	{
	  if (m_q[i]->IsFull()) return;
	}
      m_transmitter[port]->SendXON();
    }
}

void
TocinoNetDeviceReceiver::Receive(Ptr<Packet> p)
{
  unsigned int tx_port;

  // XON packet enables transmission on this port
  if (p->IsXON())
    {
      m_transmitter[port]->SetXState(XON);
      m_transmitter[port]->Transmit();
      return;
    }

  // XOFF packet disables transmission on this port
  if (p->IsXOFF())
    {
      m_transmitter[port]->SetXState(XOFF);
      return;
    }

  // figure out where the packet goes and put it in the transmission queue
  tx_port = Route(p);
  m_q[tx_port]->Enqueue(p);

  // if the buffer is full, send XOFF - blocks ALL traffic on this port
  if (m_q[tx_port]->IsFull())
    {
      m_transmitter[m_port]->SendXOFF();
      m_transmitter[m_port]->Transmit();
    }
  m_transmitter[tx_port]->Transmit();
}
