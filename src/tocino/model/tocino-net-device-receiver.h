/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_RECEIVER_H__
#define __TOCINO_NET_DEVICE_RECEIVER_H__

#include <vector>
#include "ns3/net-device.h"

namespace ns3 {

class TocinoQueue;
class TocinoNetDevice;
class TocinoNetDeviceTransmitter;

class TocinoNetDeviceReceiver : public Object
{
 public:
  TocinoNetDeviceReceiver() {};
  ~TocinoNetDeviceReceiver() {};

  Ptr<NetDevice> GetNetDevice() { return m_tnd;}

  void Receive(Ptr<Packet> p);

  friend class TocinoNetDevice;
  friend class TocinoNetDeviceTransmitter;
 private:

  uint32_t m_channelNumber;
  Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice

  std::vector<Ptr<TocinoQueue> > m_queues; // packet queues to write

  bool IsBlocked();
  void CheckForUnblock(); // called from TocinNetDeviceTransmitter

  uint32_t Route(Ptr<Packet> p); // TODO: make this runtime settable
};

}
#endif /* __TOCINO_NET_DEVICE_RECEIVER_H__ */
