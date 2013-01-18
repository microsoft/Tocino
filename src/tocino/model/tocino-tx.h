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

    TocinoTx( const uint32_t, Ptr<TocinoNetDevice> );
  
    uint32_t GetPortNumber() const;

    void SetXState( const TocinoFlowControlState& );

    bool IsVCPaused( const TocinoOutputVC ) const;

    void RemotePause( const TocinoInputVC );
    void RemoteResume( const TocinoInputVC );
    
    void SetChannel( Ptr<TocinoChannel> channel );

    Ptr<NetDevice> GetNetDevice();
    
    void Transmit();
   
    bool CanAcceptFlit(
            const TocinoInputPort,
            const TocinoOutputVC ) const;

    void AcceptFlit( 
            const TocinoInputPort,
            const TocinoOutputVC,
            Ptr<Packet> );
    
    bool CanTransmitFrom( 
            const TocinoInputPort, 
            const TocinoOutputVC ) const;
    
    Ptr<const Packet> PeekNextFlit(
            const TocinoInputPort, 
            const TocinoOutputVC ) const;
    
    bool IsNextFlitHead(
            const TocinoInputPort, 
            const TocinoOutputVC ) const;

    bool IsNextFlitTail(
            const TocinoInputPort, 
            const TocinoOutputVC ) const;

    bool AllQuiet() const;
    void DumpState() const;

    private:
    
    Ptr<CallbackQueue> GetOutputQueue( 
            const TocinoInputPort,
            const TocinoOutputVC ) const;

    void SetOutputQueue( 
            const TocinoInputPort,
            const TocinoOutputVC,
            const Ptr<CallbackQueue> );
  
    void SendToChannel( Ptr<Packet> f );
    void DoTransmitFlowControl();
    void DoTransmit();
    void TransmitEnd();

    const uint32_t m_outputPortNumber;
  
    TocinoFlowControlState m_xState;
    TocinoFlowControlState m_remoteXState;
    
    TocinoVCBitSet m_doUpdateXState;

    enum TocinoTransmitterState {IDLE, BUSY} m_state;
 
    const Ptr<TocinoNetDevice> m_tnd;
    Ptr<TocinoChannel> m_channel;
    Ptr<TocinoArbiter> m_arbiter;

    // This nested class controls access to our
    // primary state variable
    class TocinoOutputQueues
    {
        private:

        // The output queues are virtualized per input, to
        // avoid head-of-line blocking.  These queues are
        // mostly for performance.
        typedef std::vector< Ptr<CallbackQueue> > OutputVCVec;
        std::vector< OutputVCVec > vec;

        public:
        
        // If you're thinking about adding another 
        // friend function here, you're wrong. -MAS
        
        friend
            TocinoTx::TocinoTx( 
                const uint32_t, Ptr<TocinoNetDevice> );

        friend Ptr<CallbackQueue>
            TocinoTx::GetOutputQueue(
                    const TocinoInputPort,
                    const TocinoOutputVC ) const;

        friend void
            TocinoTx::SetOutputQueue( 
                const TocinoInputPort,
                const TocinoOutputVC, 
                Ptr<CallbackQueue> );

    }
    m_outputQueues;
};

} // namespace ns3

#endif // __TOCINO_TX_H__
