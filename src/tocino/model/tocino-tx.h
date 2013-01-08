/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TX_H__
#define __TOCINO_TX_H__

#include <deque>
#include <stdint.h>

#include "ns3/ptr.h"
#include "ns3/packet.h"

#include "tocino-arbiter.h"
#include "tocino-flow-control.h"

namespace ns3
{
   
class NetDevice;
class TocinoChannel;
class TocinoNetDevice;
class CallbackQueue;
class TocinoArbiter;

class TocinoTx
{
public:
    TocinoTx( const uint32_t, Ptr<TocinoNetDevice>, Ptr<TocinoArbiter> );
    ~TocinoTx();
  
    uint32_t GetPortNumber() const;

    void SetXState( const TocinoFlowControlState& );

    bool IsVCPaused(const uint32_t vc);
    bool IsAnyVCPaused() const;

    void RemotePause( const uint8_t vc );
    void RemoteResume( const uint8_t vc );
    
    void SetChannel(Ptr<TocinoChannel> channel);

    Ptr<NetDevice> GetNetDevice();
    
    void Transmit();
   
    bool CanTransmitFrom( const uint32_t qnum, const uint8_t vc ) const;

    bool IsNextFlitHead( const uint32_t port, const uint8_t vc ) const;
    bool IsNextFlitHead( const TocinoQueueDescriptor qd ) const;

    bool IsNextFlitTail( const uint32_t port, const uint8_t vc ) const;
    bool IsNextFlitTail( const TocinoQueueDescriptor qd ) const;

    void SetQueue( uint32_t, uint8_t, Ptr<CallbackQueue> );

    void DumpState();

private:
    const uint32_t m_portNumber;
  
    TocinoFlowControlState m_xState;
    TocinoFlowControlState m_remoteXState;
    
    TocinoVCBitSet m_doUpdateXState;

    enum TocinoTransmitterState {IDLE, BUSY} m_state;
 
    const Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice

    typedef std::vector< Ptr<CallbackQueue> > VCVec;
    typedef std::vector< VCVec > TxQueueVec;

    TxQueueVec m_queues;

    Ptr<TocinoChannel> m_channel; // link to channel

    void SendToChannel( Ptr<Packet> f );

    void DoTransmitFlowControl();

    // FIXME: Out parameters are ugly. Replace this with
    // std::tuple and std::tie ASAP so we can return
    // both the packet and the boolean at once.
    Ptr<Packet> DequeueHelper( const uint32_t, const uint8_t, bool& );

    void DoTransmit();

    void TransmitEnd(); // can this be private? needs to be invoked by Simulator::Schedule()

    Ptr<TocinoArbiter> m_arbiter;
};

} // namespace ns3

#endif // __TOCINO_TX_H__
