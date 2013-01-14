/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TX_H__
#define __TOCINO_TX_H__

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

    bool IsVCPaused( const uint32_t ) const;

    void RemotePause( const uint32_t );
    void RemoteResume( const uint32_t );
    
    void SetChannel( Ptr<TocinoChannel> channel );

    Ptr<NetDevice> GetNetDevice();
    
    void Transmit();
   
    bool CanAcceptFlit( const uint32_t, const uint32_t ) const;
    void AcceptFlit( const uint32_t, const uint32_t, Ptr<Packet> );

    bool CanTransmitFrom( const uint32_t, const uint32_t ) const;
    bool CanTransmitFrom( const TocinoQueueDescriptor ) const;

    bool IsNextFlitHead( const uint32_t, const uint32_t ) const;
    bool IsNextFlitHead( const TocinoQueueDescriptor ) const;

    bool IsNextFlitTail( const uint32_t, const uint32_t ) const;
    bool IsNextFlitTail( const TocinoQueueDescriptor ) const;
    
    bool AllQuiet() const;
    void DumpState() const;

private:
    const uint32_t m_outputPortNumber;
  
    TocinoFlowControlState m_xState;
    TocinoFlowControlState m_remoteXState;
    
    TocinoVCBitSet m_doUpdateXState;

    enum TocinoTransmitterState {IDLE, BUSY} m_state;
 
    const Ptr<TocinoNetDevice> m_tnd;

    // The outputQueues are virtualized per input, to
    // avoid head-of-line blocking.
    //
    // These queues are mostly for performance.
    typedef std::vector< Ptr<CallbackQueue> > OutputVCVec;
    std::vector< OutputVCVec > m_outputQueues;

    Ptr<TocinoChannel> m_channel; // link to channel

    void SendToChannel( Ptr<Packet> f );

    void DoTransmitFlowControl();

    void DoTransmit();

    void TransmitEnd();

    Ptr<const Packet>
        PeekNextFlit( const uint32_t, const uint32_t ) const;

    Ptr<TocinoArbiter> m_arbiter;
};

} // namespace ns3

#endif // __TOCINO_TX_H__
