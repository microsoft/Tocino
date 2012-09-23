/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"

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
                   "The transmission data rate to be provided to devices connected to the channel",
                   DataRateValue (DataRate (1000000000)), // 1Gbps
                   MakeDataRateAccessor (&TocinoChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&TocinoChannel::m_delay),
                   MakeTimeChecker ());
  return tid;
}

  TocinoChannel::TocinoChannel()
    : m_nDevices(2)
{
  m_tx = NULL;
  m_rx = NULL;
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
TocinoChannel::GetDevice (TocinoChannelDevice i)
{
  if (i == TX)
    {
      if (m_tx == NULL) return NULL;
      return m_tx->NetDevice;
    }
  else
    {
      if (m_rx == NULL) return NULL;
      return m_rx->NetDevice;
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

  transmit_time = (p->Size() * 8) / m_bps;
  Simulator::Schedule(transmit_time, TocinoChannel::TransmitEnd, p);
  return true;
}

void
TocinoChannel::TransmitEnd(Ptr<Packet> p)
{
  if (m_rx == NULL)
    {
      NS_LOG_WARN ("TocinoChannel missing receiver");
      return;
    }

  m_state = IDLE; // wire can be pipelined
  Simulator::ScheduleWithContext(m_rx->GetNode()->GetId(), // need to look at this
                                 m_delay,
                                 m_rx->Receive,
                                 p);
}

} // namespace ns3

