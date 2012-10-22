/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_H__
#define __TOCINO_NET_DEVICE_H__

#include <deque>

#include "ns3/net-device.h"

#include "tocino-address.h"
#include "tocino-flit-header.h"

namespace ns3
{

class Node;
class TocinoChannel;
class TocinoTx;
class TocinoRx;
class CallbackQueue;
class Queue;

class TestEjectFlit;

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
    virtual bool SendFrom(Ptr<Packet> packet, const Address& source, const Address& dest,uint16_t protocolNumber);
    virtual Ptr<Node> GetNode( void ) const;
    virtual void SetNode( Ptr<Node> node );
    virtual bool NeedsArp( void ) const;
    virtual void SetReceiveCallback( NetDevice::ReceiveCallback cb );
    virtual void SetPromiscReceiveCallback( PromiscReceiveCallback cb );
    virtual bool SupportsSendFrom( void ) const;
    
    void Initialize();
    void SetTxChannel(Ptr<TocinoChannel> c, uint32_t port);
    void SetRxChannel(Ptr<TocinoChannel> c, uint32_t port);

    TocinoRx* GetReceiver(uint32_t p) {return m_receivers[p];}
    TocinoTx* GetTransmitter(uint32_t p) {return m_transmitters[p];}
    
    friend class TocinoRx;
    friend class TocinoTx;

    static std::deque< Ptr<Packet> > Flitter(
            const Ptr<Packet>,
            const TocinoAddress&,
            const TocinoAddress&,
            const TocinoFlitHeader::Type );
    
private:
    static const uint32_t NPORTS = 7;
    static const uint32_t NVCS = 2;
    
    // disable copy and copy-assignment
    TocinoNetDevice& operator=( const TocinoNetDevice& );
    TocinoNetDevice( const TocinoNetDevice& );
        
    void InjectFlits(); // Attempt to send m_currentFlits
    
    friend class TestEjectFlit;
    void EjectFlit(Ptr<Packet>); // this gets called by a TocinoTx to eject a Packet

    uint32_t injectionPortNumber() const { return m_nPorts-1; }

    Ptr<Node> m_node;
    uint32_t m_ifIndex;
        
    static const uint16_t DEFAULT_MTU = 1500;
    uint32_t m_mtu;
    
    TocinoAddress m_address;

    // packets incoming via SendFrom
    std::deque< Ptr<Packet> > m_packetQueue;

    // current flits to be sent 
    std::deque< Ptr<Packet> > m_outgoingFlits;
 
    // state for EjectFlit
    Ptr<Packet> m_incomingPacket;
    TocinoAddress m_incomingSource;

    NetDevice::ReceiveCallback m_rxCallback;
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;

    uint32_t m_nPorts; // port count must include injection/ejection port
    uint32_t m_nVCs; // number of virtual channels on each port
    std::vector< Ptr <CallbackQueue> > m_queues;
    std::vector< TocinoTx* > m_transmitters;
    std::vector< TocinoRx* > m_receivers;
};

} // namespace ns3

#endif // __TOCINO_NET_DEVICE_H__
