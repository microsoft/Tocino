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

  TocinoChannel(Ptr<TocinoNetDeviceTransmitter> transmitter, Ptr<TocinoNetDeviceReceiver> receiver);

  bool TransmitStart (Ptr<Packet> p);
  DataRate GetDataRate() {return m_bps;}

  uint32_t GetNDevices() const {return 2;}

  Ptr<NetDevice> GetDevice( uint32_t i ) {return (i == 0)? m_transmitter->NetDevice:m_receiver->NetDevice;}

private:
  void TransmitEnd ();

  // channel parameters
  Time m_delay;
  DataRate m_bps;

  Ptr<TocinoNetDeviceTransmitter> m_transmitter;
  Ptr<TocinoNetDeviceReceiver> m_receiver;

  enum TocinoChannelState = {IDLE, BUSY};
  TocinoChannelState m_state = IDLE;
};

}

#endif /* __TOCINO_CHANNEL_H__ */

