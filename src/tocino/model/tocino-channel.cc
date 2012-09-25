/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/simulator.h"
#include "ns3/log.h"
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

TocinoChannel::TocinoChannel()
{
  m_state = IDLE;
}

void
TocinoChannel::SetTransmitter(Ptr<TocinoNetDeviceTransmitter> tx)
{
  if (m_tx != NULL)
    {
      NS_LOG_WARN ("TocinoChannel::SetTransmitter(): redefining transmitter");
    }
  m_tx = tx;
}

void
TocinoChannel::SetReceiver(Ptr<TocinoNetDeviceReceiver> rx)
{
  if (m_rx != NULL)
    {
      NS_LOG_WARN ("TocinoChannel::SetReceiver(): redefining receiver");
    }
  m_rx = rx;
}

Ptr<NetDevice>
TocinoChannel::GetDevice (uint32_t i) const
{
  if (i == TX)
    {
      if (m_tx == NULL) return NULL;
      return m_tx->GetNetDevice();
    }
  else
    {
      if (m_rx == NULL) return NULL;
      return m_rx->GetNetDevice();
    }
}

bool
TocinoChannel::TransmitStart (Ptr<Packet> p)
{
  Time transmit_time;

  if (m_state != IDLE)
    {
      NS_LOG_WARN ("TocinoChannel::TransmitStart(): m_state is not IDLE");
      return false;
    }
  m_packet = p;

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
  // Simulator::ScheduleWithContext(m_rx->GetNode()->GetId(), // need to look at this
  //                                m_delay,
  //                                m_rx->Receive,
  //                                m_packet);
}

} // namespace ns3

