/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_TRANSMITTER_H__
#define __TOCINO_NET_DEVICE_TRANSMITTER_H__

#include "ns3/tocino-net-device.h"

namespace ns3 {

class TocinoNetDeviceTransmiter
{
 public:
  TocinoNetDeviceTransmitter(<Ptr>TocinoNetDevice nd,
			     uint32_t port, 
			     uint32_t n_ports);
  ~TocinoNetDeviceTransmitter();

  enum TocinoFlowControlState = {XOFF, XON};
  void SetXState(TocinoFlowControlState s) { m_xstate = s;}
  TocinoFlowState GetXState() { return m_xstate;}
  void Transmit(Ptr<Packet> p);
  void DefinePortLinkage(uint32_t port,
                         Ptr<TocinoNetDeviceReceiver> receiver,
                         Ptr<TocinoQueue> q);
  void SetChannel(Ptr<TocinoChannel> channel) {m_channel = channel;}
  void SendXOFF() { m_pending_xoff = true;}
  void SendXON() { m_pending_xon = true;}

 private:
  const uint32_t MAX_PORTS = 7;

  uint32_t m_port;
  uint32_t m_n_ports;

  TocinoFlowControlState m_xstate = XON;

  enum TocinoTransmitterState = {IDLE, BUSY};
  TocinoTransmitterState m_state = IDLE;
  
  bool m_pending_xon = false;
  bool m_pending_xoff = false;

  Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
  Ptr<TocinoNetDeviceReceiver> m_receivers[MAX_PORTS]; // links to receivers TBD dynamically allocated
  Ptr<TocinoQueue> m_queues[MAX_PORTS]; // links to queues
  Ptr<TocinoChannel> m_channel; // link to channel

  void TransmitEnd(); // can this be private? needs to be invoked by Simulation::Schedule()
  uint32_t Arbitrate();
}

}
#endif /* __TOCINO_NET_DEVICE_TRANSMITTER_H__ */
