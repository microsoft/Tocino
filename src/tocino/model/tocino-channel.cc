/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"

#include "tocino-channel.h"
#include "tocino-net-device-transmitter.h"
#include "tocino-net-device-receiver.h"

NS_LOG_COMPONENT_DEFINE ("TocinoChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED( TocinoChannel );

TypeId TocinoChannel::GetTypeId( void )
{
  static TypeId tid = TypeId("ns3::TocinoChannel")
    .SetParent<Channel>()
    .AddConstructor<TocinoChannel>()
    .AddAttribute ("DataRate", 
                   "The data transmission rate of the channel",
                   DataRateValue (DataRate ("10Gbps")),
                   MakeDataRateAccessor (&TocinoChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&TocinoChannel::m_delay),
                   MakeTimeChecker ());
  return tid;
}

Ptr<NetDevice>
TocinoChannel::GetDevice (uint32_t i) const
{
  if (i == TX)
    {
      if (m_tx == NULL) return 0;
      return m_tx->GetNetDevice();
    }
  else
    {
      if (m_rx == NULL) return 0;
      return m_rx->GetNetDevice();
    }
}

bool
TocinoChannel::TransmitStart (Ptr<Packet> p)
{
  Time transmit_time;

  if (m_state != IDLE)
    {
      NS_LOG_ERROR ("TocinoChannel::TransmitStart(): m_state is not IDLE");
      return false;
    }
  m_packet = p;

  NS_LOG_INFO("starting packet transmission");
  transmit_time = GetTransmissionTime(p);
  Simulator::Schedule(transmit_time, &TocinoChannel::TransmitEnd, this);
  return true;
}

Time
TocinoChannel::GetTransmissionTime(Ptr<Packet> p)
{
  return Seconds(m_bps.CalculateTxTime(p->GetSerializedSize()*8));
}

void
TocinoChannel::TransmitEnd()
{
  m_state = IDLE; // wire can be pipelined
  Simulator::ScheduleWithContext(m_rx->GetNetDevice()->GetNode()->GetId(), // need to look at this
                                 m_delay,
                                 &TocinoNetDeviceReceiver::Receive,
                                 m_rx,
                                 m_packet);
}

} // namespace ns3

