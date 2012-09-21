/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_RECEIVER_H__
#define __TOCINO_NET_DEVICE_RECEIVER_H__

#include "ns3/tocino-net-device.h"

namespace ns3 {

class TocinoNetDeviceReceiver
{
 public:
  TocinoNetDeviceReceiver(<Ptr>TocinoNetDevice nd, unsigned int port, unsigned int n_ports);
  ~TocinoNetDeviceReceiver();

  void Receive(Ptr<Packet> p);
  void PortLinkage(unsigned int port,
                  Ptr<TocinoNetDeviceTransmitter> transmitter,
                  Ptr<TocinoQueue> q);
  void CheckForUnblock();

 private:
  const unsigned int MAX_PORTS = 7;

  unsigned int m_port;
  unsigned int m_n_ports;

  unsigned int Route(Ptr<Packet> p); // might want this to be a function pointer w/ a setter
  Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
  Ptr<TocinoNetDeviceTransmitter> m_transmitter[MAX_PORTS]; // links to transmitters
  Ptr<TocinoQueue> m_q[MAX_PORTS]; // links to queues
}

}
#endif /* __TOCINO_NET_DEVICE_RECEIVER_H__ */
