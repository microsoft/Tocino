/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_CHANNEL_H__
#define __TOCINO_CHANNEL_H__

#include "ns3/nstime.h"
#include "ns3/channel.h"

namespace ns3 {

class Packet;
class TocinoNetDeviceTransmitter;
class TocinoNetDeviceReceiver;
class DataRate;
class Time;

class TocinoChannel : public Channel
{
public:
  static TypeId GetTypeId();

  TocinoChannel() { m_state = IDLE;}
  ~TocinoChannel() {};

  bool TransmitStart (Ptr<Packet> p);
  Time GetTransmissionTime(Ptr<Packet> p);

  void SetTransmitter(Ptr<TocinoNetDeviceTransmitter> tx) {m_tx = tx;}
  void SetReceiver(Ptr<TocinoNetDeviceReceiver> rx) {m_rx = rx;}
  uint32_t GetNDevices() const {return 2;}

  enum TocinoChannelDevice {TX, RX};
  Ptr<NetDevice> GetDevice(uint32_t i) const;

private:
  void TransmitEnd ();

  // channel parameters
  Time m_delay;
  DataRate m_bps;

  Ptr<Packet> m_packet;

  Ptr<TocinoNetDeviceTransmitter> m_tx;
  Ptr<TocinoNetDeviceReceiver> m_rx;

  enum TocinoChannelState {IDLE, BUSY};
  TocinoChannelState m_state;
};

} // namespace ns3

#endif /* __TOCINO_CHANNEL_H__ */

