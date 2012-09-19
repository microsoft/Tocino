/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_H__
#define __TOCINO_NET_DEVICE_H__

#include <stdint.h>
#include <vector>

#include "ns3/data-rate.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"

#include "tocino-address.h"

namespace ns3 {

class TocinoChannel;

class TocinoNetDevice : public NetDevice
{
    public:

    static TypeId GetTypeId( void );

    TocinoNetDevice();
    virtual ~TocinoNetDevice();

    virtual void SetIfIndex( const uint32_t index );
    virtual uint32_t GetIfIndex( void ) const;
    virtual Ptr<Channel> GetChannel( void ) const;
    virtual bool SetMtu( const uint16_t mtu );
    virtual uint16_t GetMtu( void ) const;
    virtual void SetAddress( Address address );
    virtual Address GetAddress( void ) const;
    virtual bool IsLinkUp( void ) const;
    virtual void AddLinkChangeCallback( Callback<void> callback );
    virtual bool IsBroadcast( void ) const;
    virtual Address GetBroadcast( void ) const;
    virtual bool IsMulticast( void ) const;
    virtual Address GetMulticast( Ipv4Address a ) const;
    virtual Address GetMulticast( Ipv6Address a ) const;
    virtual bool IsPointToPoint( void ) const;
    virtual bool IsBridge( void ) const;
    virtual bool Send( Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber );
    virtual bool SendFrom( Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber );
    virtual Ptr<Node> GetNode( void ) const;
    virtual void SetNode( Ptr<Node> node );
    virtual bool NeedsArp( void ) const;
    virtual void SetReceiveCallback( NetDevice::ReceiveCallback cb );
    virtual void SetPromiscReceiveCallback( PromiscReceiveCallback cb );
    virtual bool SupportsSendFrom( void ) const;
    void AddChannel( Ptr<TocinoChannel> c );
    
    private:

    // disable copy and copy-assignment
    TocinoNetDevice& operator=( const TocinoNetDevice& );
    TocinoNetDevice( const TocinoNetDevice& );
    
    DataRate m_bps;
  
    Ptr<Node> m_node;
    uint32_t m_ifIndex;
    
    static const uint16_t DEFAULT_MTU = 1500;
    uint32_t m_mtu;

    TocinoAddress m_address;
  
    NetDevice::ReceiveCallback m_rxCallback;
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;

    std::vector< Ptr<TocinoChannel> > m_channels;
};

}

#endif /* __TOCINO_NET_DEVICE_H__ */

