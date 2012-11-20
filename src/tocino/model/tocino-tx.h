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
   
    void Pause();
    void Resume();
    bool IsPaused() const;

    void RemotePause();
    void RemoteResume();
    
    void SetChannel(Ptr<TocinoChannel> channel);

    Ptr<NetDevice> GetNetDevice();
    
    void Transmit();
   
    bool IsQueueEmpty( uint32_t qnum ) const;
    bool IsQueueNotEmpty( uint32_t qnum ) const;

    Ptr<const Packet> PeekNextFlit( uint32_t qnum ) const;
    bool IsNextFlitTail( uint32_t qnum ) const;

    void SetQueue( uint32_t, Ptr<CallbackQueue> );

private:
    const uint32_t m_portNumber;
    
    TocinoFlowControl::State m_xstate;

    enum TocinoTransmitterState {IDLE, BUSY};
    TocinoTransmitterState m_state;
  
    bool m_remoteResumeRequested;
    bool m_remotePauseRequested;

    const Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice

    std::vector< Ptr <CallbackQueue> > m_queues; // links to queues

    Ptr<TocinoChannel> m_channel; // link to channel

    void SendToChannel( Ptr<Packet> f );

    void DoTransmitPause();
    void DoTransmitResume();

    void DoTransmit();

    void TransmitEnd(); // can this be private? needs to be invoked by Simulator::Schedule()

    Ptr<TocinoArbiter> m_arbiter;
};

} // namespace ns3

#endif // __TOCINO_TX_H__