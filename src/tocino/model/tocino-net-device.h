/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_H__
#define __TOCINO_NET_DEVICE_H__

#include "ns3/net-device.h"

#include "tocino-address.h"

class TocinoFlitLoopback;

namespace ns3
{

class Node;
class TocinoChannel;
class TocinoTx;
class TocinoRx;
class CallbackQueue;
class Queue;

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
    
    uint32_t GetEjectedFlitCount() {return m_nEjectedFlits;}

    friend class TocinoRx;
    friend class TocinoTx;
    friend class ::TocinoFlitLoopback; // test needs access to InjectFlit()

    static std::vector< Ptr<Packet> > Flitter(
            const Ptr<Packet>,
            const TocinoAddress&,
            const TocinoAddress& );
    
    static Ptr<Packet> Deflitter( 
            const std::vector< Ptr<Packet> >&,
            /* out */ TocinoAddress&, 
            /* out */ TocinoAddress& );
    
private:
    static const uint32_t NPORTS = 7;
    
    // disable copy and copy-assignment
    TocinoNetDevice& operator=( const TocinoNetDevice& );
    TocinoNetDevice( const TocinoNetDevice& );
        
    bool InjectFlit(Ptr<Packet>); // this gets called to inject a Packet
    bool EjectFlit(Ptr<Packet>); // this gets called by a TocinoTx to eject a Packet
    
    Ptr<Node> m_node;
    uint32_t m_ifIndex;
        
    static const uint16_t DEFAULT_MTU = 1500;
    uint32_t m_mtu;
    
    TocinoAddress m_address;

    Ptr<Queue> m_packetQueue;

    NetDevice::ReceiveCallback m_rxCallback;
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;
        
    uint32_t m_nPorts; // port count must include injection/ejection port
    uint32_t m_nVCs; // number of virtual channels on each port
    std::vector< Ptr <CallbackQueue> > m_queues;
    std::vector< TocinoTx* > m_transmitters;
    std::vector< TocinoRx* > m_receivers;

    uint32_t m_nEjectedFlits;
};

} // namespace ns3

#endif // __TOCINO_NET_DEVICE_H__
