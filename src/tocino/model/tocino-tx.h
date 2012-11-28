/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TX_H__
#define __TOCINO_TX_H__

#include <stdint.h>

#include "ns3/ptr.h"
#include "ns3/packet.h"

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
   
    void SetXState( const TocinoFlowControlState& );

    bool IsAnyVCPaused() const;

    void RemotePause( const uint8_t vc );
    void RemoteResume( const uint8_t vc );
    
    void SetChannel(Ptr<TocinoChannel> channel);

    Ptr<NetDevice> GetNetDevice();
    
    void Transmit();
   
    bool CanTransmitFrom( uint32_t qnum ) const;

    bool IsNextFlitTail( uint32_t qnum ) const;

    void SetQueue( uint32_t, Ptr<CallbackQueue> );

private:
    const uint32_t m_portNumber;
  
    TocinoFlowControlState m_xState;
    TocinoFlowControlState m_remoteXState;
    
    TocinoVCBitSet m_doUpdateXState;

    enum TocinoTransmitterState {IDLE, BUSY} m_state;
 
    const Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice

    std::vector< Ptr <CallbackQueue> > m_queues; // links to queues

    Ptr<TocinoChannel> m_channel; // link to channel

    void SendToChannel( Ptr<Packet> f );

    void DoTransmitFlowControl();

    void DoTransmit();

    void TransmitEnd(); // can this be private? needs to be invoked by Simulator::Schedule()

    Ptr<TocinoArbiter> m_arbiter;
};

} // namespace ns3

#endif // __TOCINO_TX_H__
