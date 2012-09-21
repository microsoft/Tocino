/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_TRANSMITTER_H__
#define __TOCINO_NET_DEVICE_TRANSMITTER_H__

#include "ns3/tocino-net-device.h"

namespace ns3 {

class TocinoNetDeviceTransmiter
{
 public:
  TocinoNetDeviceTransmitter(<Ptr>TocinoNetDevice nd,
			     unsigned int port, 
			     unsigned int n_ports);
  ~TocinoNetDeviceTransmitter();

  enum TocinoFlowControlState = {XOFF, XON};
  void SetXState(TocinoFlowControlState s) { m_xstate = s;}
  TocinoFlowState GetXState() { return m_xstate;}
  void Transmit(Ptr<Packet> p);
  void PortLinkage(unsigned int port,
		   Ptr<TocinoNetDeviceReceiver> receiver,
		   Ptr<TocinoQueue> q);
  void SetChannel(Ptr<TocinoChannel> channel) {m_channel = channel;}
  void SendXOFF() { m_pending_xoff = true;}
  void SendXON() { m_pending_xon = true;}

 private:
  const unsigned int MAX_PORTS = 7;

  unsigned int m_port;
  unsigned int m_n_ports;

  TocinoFlowControlState m_xstate = XON;

  enum TocinoTransmitterState = {IDLE, BUSY};
  TocinoTransmitterState m_state = IDLE;
  
  bool m_pending_xon = false;
  bool m_pending_xoff = false;

  Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
  Ptr<TocinoNetDeviceReceiver> m_receiver[MAX_PORTS]; // links to receivers TBD dynamically allocated
  Ptr<TocinoQueue> m_q[MAX_PORTS]; // links to queues
  Ptr<TocinoChannel> m_channel; // link to channel

  void TransmitEnd(); // can this be private? needs to be invoked by Simulation::Schedule()
  unsigned int Arbitrate();
}

}
#endif /* __TOCINO_NET_DEVICE_TRANSMITTER_H__ */
