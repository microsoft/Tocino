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
#include "tocino-channel.h"

namespace ns3 {

class TocinoNetDevice : public NetDevice
{
    public:

    TocinoNetDevice() :
        m_node( 0 ),
        m_ifIndex( 0 ),
        m_mtu( DEFAULT_MTU )
    {}
        
    virtual ~TocinoNetDevice() {};

    virtual void SetIfIndex( const uint32_t index )
    {
        m_ifIndex = index;
    }

    virtual uint32_t GetIfIndex( void ) const
    { 
        return m_ifIndex;
    }

    virtual Ptr<Channel> GetChannel( void ) const
    {
        return 0;
    }
    
    virtual bool SetMtu( const uint16_t mtu )
    {
        m_mtu = mtu;
        return true;
    }

    virtual uint16_t GetMtu( void ) const
    {
        return m_mtu;
    }

    virtual void SetAddress( Address address )
    {
        m_address = TocinoAddress::ConvertFrom( address );
    }

    virtual Address GetAddress( void ) const
    {
        return m_address;
    }

    virtual bool IsLinkUp( void ) const
    {
        return true;
    }

    virtual void AddLinkChangeCallback( Callback<void> callback )
    {
        //Do nothing for now 
    }

    virtual bool IsBroadcast( void ) const
    {
        return true;
    }

    virtual Address GetBroadcast( void ) const
    {
        return TocinoAddress::ConvertFrom( Mac48Address ("ff:ff:ff:ff:ff:ff") );
    }

    virtual bool IsMulticast( void ) const
    {
        return true;
    }

    virtual Address GetMulticast( Ipv4Address a ) const
    {
        return TocinoAddress::ConvertFrom( Mac48Address::GetMulticast( a ) );
    }
    
    virtual Address GetMulticast( Ipv6Address a ) const
    {
        return TocinoAddress::ConvertFrom( Mac48Address::GetMulticast( a ) );
    }

    virtual bool IsPointToPoint( void ) const
    {
        return false;
    }

    virtual bool IsBridge( void ) const
    {
        return false;
    }
    
    virtual bool Send( Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber )
    {
        return SendFrom( packet, m_address, dest, protocolNumber );
    }

    virtual bool SendFrom( Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber )
    {
        // TODO
        // TODO
        // TODO
        return false;
        // TODO
        // TODO
        // TODO
    }

    virtual Ptr<Node> GetNode( void ) const
    {
        return m_node;
    }

    virtual void SetNode( Ptr<Node> node )
    {
        m_node = node;
    }
    
    virtual bool NeedsArp( void ) const
    {
        return true;
    }

    virtual void SetReceiveCallback( NetDevice::ReceiveCallback cb )
    {
        m_rxCallback = cb;
    }

    virtual void SetPromiscReceiveCallback( PromiscReceiveCallback cb )
    {
        m_promiscRxCallback = cb;
    }

    virtual bool SupportsSendFrom( void ) const
    {
        return true;
    }
 
    private:

    // disable copy and copy-assignment
    TocinoNetDevice &operator=( const TocinoNetDevice& );
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

