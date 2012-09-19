/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-channel.h"
#include "tocino-net-device.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED( TocinoChannel );

TypeId TocinoChannel::GetTypeId( void )
{
    static TypeId tid = TypeId("ns3::TocinoChannel")
        .SetParent<Channel>()
        .AddConstructor<TocinoChannel>();
    return tid;
}

TocinoChannel::TocinoChannel()
    : m_nDevices(0)
{}

void TocinoChannel::Attach( Ptr<TocinoNetDevice> device )
{}

bool TocinoChannel::TransmitStart( Ptr<Packet> p, uint32_t srcId )
{
    return false;
}

//bool TocinoChannel::TransmitStart (Ptr<Packet> p, Ptr<TocinoNetDevice> src, Time txTime);

uint32_t TocinoChannel::GetNDevices() const
{
   return m_nDevices; 
}

Ptr<NetDevice> TocinoChannel::GetDevice( uint32_t i ) const
{
    return 0;
}

Ptr<TocinoNetDevice> TocinoChannel::GetTocinoNetDevice( uint32_t i ) const
{
    return 0;
}

}

