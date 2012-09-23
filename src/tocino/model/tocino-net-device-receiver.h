/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_RECEIVER_H__
#define __TOCINO_NET_DEVICE_RECEIVER_H__

#include "ns3/tocino-net-device.h"

namespace ns3 {

class TocinoNetDeviceReceiver
{
 public:
  TocinoNetDeviceReceiver(<Ptr>TocinoNetDevice nd, uint32_t port, uint32_t nPorts);
  ~TocinoNetDeviceReceiver();

  void Receive(Ptr<Packet> p);
  void DefinePortLinkage(uint32_t port,
                         Ptr<TocinoNetDeviceTransmitter> transmitter,
                         Ptr<TocinoQueue> q);
  void CheckForUnblock();

 private:
  const uint32_t MAX_PORTS = 7;

  uint32_t m_port;
  uint32_t m_nPorts;

  uint32_t Route(Ptr<Packet> p); // might want this to be a function pointer w/ a setter
  Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
  Ptr<TocinoNetDeviceTransmitter> m_transmitters[MAX_PORTS]; // links to transmitters
  Ptr<TocinoQueue> m_queues[MAX_PORTS]; // links to queues
}

}
#endif /* __TOCINO_NET_DEVICE_RECEIVER_H__ */
