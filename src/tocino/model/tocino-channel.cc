/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include "ns3/node.h"

#include "tocino-channel.h"
#include "tocino-rx.h"
#include "tocino-tx.h"
#include "tocino-net-device.h"

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

TocinoChannel::~TocinoChannel()
{};

Time TocinoChannel::GetTransmissionTime(Ptr<Packet> p)
//Time TocinoChannel::GetTransmissionTime(Ptr<const Packet> p)
{
    return Seconds(m_bps.CalculateTxTime(p->GetSize()*8));
}

void TocinoChannel::SetNetDevice(Ptr<TocinoNetDevice> tnd)
{
    m_tnd = tnd;
}

void TocinoChannel::SetTransmitter(TocinoTx* tx)
{
    m_tx = tx;
}

void TocinoChannel::SetReceiver(TocinoRx* rx)
{
    m_rx = rx;
}

uint32_t TocinoChannel::GetNDevices() const
{
    return 2;
}

Ptr<NetDevice> 
TocinoChannel::GetDevice(uint32_t i) const
{
    if (i == TX_DEV)
    {
        if (m_tx == NULL) return 0;
        return m_tx->GetNetDevice();
    }
    if (m_rx == NULL) return 0;
    return m_rx->GetNetDevice();
}

bool
TocinoChannel::TransmitStart (Ptr<Packet> p)
//TocinoChannel::TransmitStart (Ptr<const Packet> p)
{
    Time transmit_time;
    
    if (m_state != IDLE)
    {
        NS_LOG_ERROR ("TocinoChannel::TransmitStart(): m_state is not IDLE");
        return false;
    }
    m_packet = p;
    
    //char str[64];
    //sprintf(str,"starting transmission of 0x%08x", (uint32_t)PeekPointer(p));
    //NS_LOG_INFO(str);
 
    transmit_time = GetTransmissionTime(p);
    Simulator::Schedule(transmit_time, &TocinoChannel::TransmitEnd, this);
    return true;
}

void 
TocinoChannel::TransmitEnd ()
{
    m_state = IDLE; // wire can be pipelined
    
    Simulator::ScheduleWithContext(m_rx->GetNetDevice()->GetNode()->GetId(),
                                   m_delay,
                                   &TocinoRx::Receive,
                                   m_rx,
                                   m_packet);
}

} // namespace ns3

