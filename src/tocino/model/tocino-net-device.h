/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_NET_DEVICE_H__
#define __TOCINO_NET_DEVICE_H__

#include <deque>

#include "ns3/net-device.h"

#include "tocino-address.h"
#include "tocino-flit-header.h"
#include "tocino-misc.h"

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
   
    virtual void DoDispose();

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
    
    void SetChannel( uint32_t, Ptr<TocinoChannel> );
    Ptr<TocinoChannel> GetChannel( uint32_t );

    TocinoAddress GetTocinoAddress( void ) const;

    TocinoRx* GetReceiver( const TocinoInputPort ) const;
    TocinoTx* GetTransmitter( const TocinoOutputPort ) const;
   
    uint32_t GetNPorts() const;
    uint32_t GetNVCs() const;

    // Get the injection/ejection port
    uint32_t GetHostPort() const;

    std::deque< Ptr<Packet> > Flitter(
            const Ptr<Packet>,
            const TocinoAddress&,
            const TocinoAddress&,
            const TocinoFlitHeader::Type );
   
    bool AllQuiet() const;
   
    void DumpState() const;

    // Attempt to send m_currentFlits
    void TrySendFlits();

    // called by TocinoTx to eject a flit
    void EjectFlit(Ptr<Packet>);

    const TypeId& GetRouterTypeId() const;
    const TypeId& GetArbiterTypeId() const;

private:
    // disable copy and copy-assignment
    TocinoNetDevice& operator=( const TocinoNetDevice& );
    TocinoNetDevice( const TocinoNetDevice& );
       
    void InjectFlit( Ptr<Packet> ) const; // send one flit

    Ptr<Node> m_node;
    uint32_t m_ifIndex;
        
    static const uint16_t DEFAULT_MTU = 1500;
    uint32_t m_mtu;
    
    TocinoAddress m_address;

    typedef std::deque< Ptr<Packet> > FlittizedPacket;

    // current flits to be sent 
    FlittizedPacket m_outgoingFlits;

    uint32_t m_outgoingFlitsMaxSize;
    
    // state for EjectFlit
    std::vector< Ptr<Packet> > m_incomingPackets;
    std::vector< TocinoAddress > m_incomingSources;
 
    NetDevice::ReceiveCallback m_rxCallback;
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;
    
    static const uint32_t DEFAULT_NPORTS = 7;

#ifndef TOCINO_VC_STRESS_MODE
    static const uint32_t DEFAULT_NVCS = 2;
#else
    static const uint32_t DEFAULT_NVCS = TOCINO_MAX_VCS;
#endif

    uint32_t m_nPorts; // port count must include injection/ejection port
    uint32_t m_nVCs; // number of virtual channels on each port

    std::vector< TocinoTx* > m_transmitters;
    std::vector< TocinoRx* > m_receivers;
    
    TypeId m_routerTypeId;
    TypeId m_arbiterTypeId;

#ifdef TOCINO_VC_STRESS_MODE
    uint32_t m_flowCounter;
#endif
};

} // namespace ns3

#endif // __TOCINO_NET_DEVICE_H__
