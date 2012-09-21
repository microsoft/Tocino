/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-channel.h"
#include "tocino-net-device.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED( TocinoChannel );

TypeId TocinoChannel::GetTypeId( void )
{
  static TypeId tid = TypeId("ns3::TocinoChannel")
    .SetParent<Channel>()
    .AddConstructor<TocinoChannel>()
    .AddAttribute ("DataRate", 
                   "The transmission data rate to be provided to devices connected to the channel",
                   DataRateValue (DataRate (0xffffffff)),
                   MakeDataRateAccessor (&TocinoChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&TocinoChannel::m_delay),
                   MakeTimeChecker ());
  return tid;
}

TocinoChannel::TocinoChannel(Ptr<TocinoNetDeviceTransmitter> transmitter, 
                             Ptr<TocinoNetDeviceReceiver> receiver)
    : m_nDevices(2)
{
  m_transmitter = transmitter;
  m_receiver = receiver;
  m_state = IDLE;
  // m_delay and m_bps are set via attribute system
}

bool TocinoChannel::TransmitStart (Ptr<Packet> p)
{
  Time transmit_time;

  if (m_state != IDLE)
    {
      NS_LOG_WARN ("TocinoChannel::TransmitStart(): m_state is not IDLE");
      return false;
    }

  transmit_time = (p->Size() * 8) / m_bps;
  Simulator::Schedule(transmit_time, TocinoChannel::TransmitEnd, p);
  return true;
}

void
TocinoChannel::TransmitEnd(Ptr<Packet> p)
{
  m_state = IDLE; // wire can be pipelined
  Simulator::ScheduleWithContext(m_receiver->GetNode()->GetId(), // need to look at this
                                 m_delay,
                                 m_receiver->Receive,
                                 p);
}

} // namespace ns3

