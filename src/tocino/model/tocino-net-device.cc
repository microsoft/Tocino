/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-net-device.h"
#include "tocino-channel.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TocinoNetDevice);

TypeId TocinoNetDevice::GetTypeId(void)
{
  static TypeId tid = TypeId( "ns3::TocinoNetDevice" )
    .SetParent<NetDevice>()
    .AddConstructor<TocinoNetDevice>();
  return tid;
}

TocinoNetDevice::TocinoNetDevice() :
    m_node( 0 ),
    m_ifIndex( 0 ),
    m_mtu( DEFAULT_MTU )
{
  const unsigned int N_PORTS = 7;
  const unsigned int N_BUFFERS = 4;

  unsigned int i, j, k;
  unsigned int index;

  // create queues - right now 1 per s/d pair
  for (i = 0; i < N_PORTS; i++)
    {
      for (j = 0; j < N_PORTS; j++)
        {
          index = (i * N_PORTS) + j;
          q[index] = new TocinoQueue(N_BUFFERS);
        }
    }
}
        
TocinoNetDevice::~TocinoNetDevice()
{}

void TocinoNetDevice::SetIfIndex( const uint32_t index )
{
    m_ifIndex = index;
}

uint32_t TocinoNetDevice::GetIfIndex( void ) const
{ 
    return m_ifIndex;
}

Ptr<Channel> TocinoNetDevice::GetChannel( void ) const
{
    return 0;
}

bool TocinoNetDevice::SetMtu( const uint16_t mtu )
{
    m_mtu = mtu;
    return true;
}

uint16_t TocinoNetDevice::GetMtu( void ) const
{
    return m_mtu;
}

void TocinoNetDevice::SetAddress( Address address )
{
    m_address = TocinoAddress::ConvertFrom( address );
}

Address TocinoNetDevice::GetAddress( void ) const
{
    return m_address;
}

bool TocinoNetDevice::IsLinkUp( void ) const
{
    return true;
}

void TocinoNetDevice::AddLinkChangeCallback( Callback<void> callback )
{
    //Do nothing for now 
}

bool TocinoNetDevice::IsBroadcast( void ) const
{
    return true;
}

Address TocinoNetDevice::GetBroadcast( void ) const
{
    return TocinoAddress::ConvertFrom( Mac48Address ("ff:ff:ff:ff:ff:ff") );
}

bool TocinoNetDevice::IsMulticast( void ) const
{
    return true;
}

Address TocinoNetDevice::GetMulticast( Ipv4Address a ) const
{
    return TocinoAddress::ConvertFrom( Mac48Address::GetMulticast( a ) );
}

Address TocinoNetDevice::GetMulticast( Ipv6Address a ) const
{
    return TocinoAddress::ConvertFrom( Mac48Address::GetMulticast( a ) );
}

bool TocinoNetDevice::IsPointToPoint( void ) const
{
    return false;
}

bool TocinoNetDevice::IsBridge( void ) const
{
    return false;
}

bool TocinoNetDevice::Send( Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber )
{
    return SendFrom( packet, m_address, dest, protocolNumber );
}

bool TocinoNetDevice::SendFrom( Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber )
{
    // TODO
    // TODO
    // TODO
    return false;
    // TODO
    // TODO
    // TODO
}

Ptr<Node> TocinoNetDevice::GetNode( void ) const
{
    return m_node;
}

void TocinoNetDevice::SetNode( Ptr<Node> node )
{
    m_node = node;
}

bool TocinoNetDevice::NeedsArp( void ) const
{
    return true;
}

void TocinoNetDevice::SetReceiveCallback( NetDevice::ReceiveCallback cb )
{
    m_rxCallback = cb;
}

void TocinoNetDevice::SetPromiscReceiveCallback( PromiscReceiveCallback cb )
{
    m_promiscRxCallback = cb;
}

bool TocinoNetDevice::SupportsSendFrom( void ) const
{
    return true;
}

void TocinoNetDevice::AddTxChannel(Ptr<TocinoChannel> c, unsigned int port)
{

}

void TocinoNetDevice::AddRxChannel(Ptr<TocinoChannel> c, unsigned int port)
{
}

}

