/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_CHANNEL_H__
#define __TOCINO_CHANNEL_H__

#include "ns3/channel.h"

namespace ns3 {

class Packet;
class TocinoNetDeviceTransmitter;
class TocinoNetDeviceReceiver;

class TocinoChannel : public Channel
{
public:

  static TypeId GetTypeId();

  TocinoChannel();
  ~TocinoChannel();

  bool TransmitStart (Ptr<Packet> p);
  DataRate GetDataRate() {return m_bps;}
  void SetTransmitter(Ptr<TocinoNetDeviceTransmitter> tx);
  void SetReceiver(Ptr<TocinoNetDeviceReceiver> rx);
  uint32_t GetNDevices() const {return 2;}

  enum TocinoChannelDevice = {TX, RX};
  Ptr<NetDevice> GetDevice( TocinoChannelDevice i );

private:
  void TransmitEnd ();

  // channel parameters
  Time m_delay;
  DataRate m_bps;

  Ptr<TocinoNetDeviceTransmitter> m_tx;
  Ptr<TocinoNetDeviceReceiver> m_rx;

  enum TocinoChannelState = {IDLE, BUSY};
  TocinoChannelState m_state = IDLE;
};

} // namespace ns3

#endif /* __TOCINO_CHANNEL_H__ */

