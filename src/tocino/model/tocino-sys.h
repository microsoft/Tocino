/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_SYS_H__
#define __TOCINO_SYS_H__

#include <stdint.h>
#include <vector>

#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

/* #include "tocino-enum.h" */
#include "tocino-address.h"
/* #include "tocino-channel.h" */
#include "callback-queue.h"
/* #include "tocino-net-device-receiver.h" */
/* #include "tocino-net-device-transmitter.h" */

namespace ns3 {
enum TocinoChannelDevice {TX, RX};
enum TocinoChannelState {IDLE, BUSY};
enum TocinoFlowControlState {XOFF, XON};

class TocinoTx;
class TocinoRx;
class TocinoNetDevice;

class TocinoChannel : public Channel
{
public:
    static TypeId GetTypeId();
    
    TocinoChannel() {m_state = IDLE;}
    ~TocinoChannel() {};
    
    bool TransmitStart(Ptr<Packet> p);
    Time GetTransmissionTime(Ptr<Packet> p) {return Seconds(m_bps.CalculateTxTime(p->GetSerializedSize()*8));}
    
    void SetNetDevice(Ptr<TocinoNetDevice> tnd) {m_tnd = tnd;}
    
    void SetTransmitter(TocinoTx* tx) {m_tx = tx;}
    void SetReceiver(TocinoRx* rx) {m_rx = rx;}
    uint32_t GetNDevices() const {return 2;}
    
    Ptr<NetDevice> GetDevice(uint32_t i) const;
    
private:
    void TransmitEnd ();
    
    // channel parameters
    Time m_delay;
    DataRate m_bps;
    
    Ptr<Packet> m_packet;
    
    Ptr<TocinoNetDevice> m_tnd;
    TocinoTx* m_tx;
    TocinoRx* m_rx;
    TocinoChannelState m_state;
};

class TocinoNetDevice : public NetDevice
{
public:
    static TypeId GetTypeId( void );
    
    TocinoNetDevice();
    ~TocinoNetDevice() {};
    
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
    
private:
    static const uint32_t NPORTS = 7;
    
    // disable copy and copy-assignment
    TocinoNetDevice& operator=( const TocinoNetDevice& );
    TocinoNetDevice( const TocinoNetDevice& );
        
    bool InjectPacket(Ptr<Packet>); // this gets called to inject a Packet
    bool EjectPacket(Ptr<Packet>); // this gets called by a TocinoTx to eject a Packet
    
    Ptr<Node> m_node;
    uint32_t m_ifIndex;
        
    static const uint16_t DEFAULT_MTU = 1500;
    uint32_t m_mtu;
    
    TocinoAddress m_address;
        
    NetDevice::ReceiveCallback m_rxCallback;
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;
        
    uint32_t m_nPorts; // port count must include injection/ejection port
    uint32_t m_nVCs; // number of virtual channels on each port
    std::vector< Ptr <CallbackQueue> > m_queues;
    std::vector< TocinoTx* > m_transmitters;
    std::vector< TocinoRx* > m_receivers;
};

class TocinoRx
{
public:
    TocinoRx(uint32_t nPorts) {m_queues.resize(nPorts);};
    ~TocinoRx() {};
    
    Ptr<NetDevice> GetNetDevice() { return m_tnd;}
    
    void Receive(Ptr<Packet> p);
    
    friend class TocinoNetDevice;
    friend class TocinoTx;
private:
    
    uint32_t m_channelNumber;
    Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
    
    std::vector< Ptr <CallbackQueue> > m_queues; // packet queues to write
    
    bool IsBlocked();
    void CheckForUnblock(); // called from TocinNetDeviceTransmitter
    
    uint32_t Route(Ptr<Packet> p); // TODO: make this runtime settable
};

class TocinoTx
{
public:
    TocinoTx(uint32_t nPorts);
    ~TocinoTx() {};
    
    void SetXState(TocinoFlowControlState s) {m_xstate = s;}
    TocinoFlowControlState GetXState() {return m_xstate;}
    
    void SetChannel(Ptr<TocinoChannel> channel) {m_channel = channel;}
    void SendXOFF() {m_pending_xoff = true;}
    void SendXON() {m_pending_xon = true;}
    
    Ptr<NetDevice> GetNetDevice() {return m_tnd;}
    
    void Transmit();
    
    friend class TocinoNetDevice;
    
private:
    uint32_t m_channelNumber;
    
    TocinoFlowControlState m_xstate;
    Ptr<Packet> m_packet;

    enum TocinoTransmitterState {IDLE, BUSY};
    TocinoTransmitterState m_state;
  
    bool m_pending_xon;
    bool m_pending_xoff;

    Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice

    std::vector< Ptr <CallbackQueue> > m_queues; // links to queues

    Ptr<TocinoChannel> m_channel; // link to channel

    void TransmitEnd(); // can this be private? needs to be invoked by Simulator::Schedule()
    uint32_t Arbitrate();
};

} // namespace ns3

#endif // __TOCINO_SYS_H__
