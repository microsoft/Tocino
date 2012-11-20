/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_RX_H__
#define __TOCINO_RX_H__

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"

#include "tocino-flow-control.h"

namespace ns3
{

class NetDevice;
class TocinoNetDevice;
class Packet;
class CallbackQueue;
class TocinoRouter;
class TocinoTx;

class TocinoRx
{
public:
    TocinoRx( const uint32_t, Ptr<TocinoNetDevice>, Ptr<TocinoRouter> );
    ~TocinoRx();
    
    Ptr<NetDevice> GetNetDevice();
    
    void SetUpstreamXState(TocinoFlowControl::State s);
    TocinoFlowControl::State GetUpstreamXState() const;
    
    bool IsQueueBlocked( uint32_t ) const;
    bool IsAnyQueueBlocked() const ;
    bool IsVCBlocked( uint8_t ) const;

    void CheckForUnblock(); // called from TocinNetDeviceTransmitter

    void SetQueue( uint32_t, Ptr<CallbackQueue> );
    
    void Receive(Ptr<Packet> p);
    
private:
    
    const uint32_t m_portNumber;

    // tracks xstate of TocinoTx on other end of channel
    TocinoFlowControl::State m_upstreamXState;

    // link to owning TocinoNetDevice
    const Ptr<TocinoNetDevice> m_tnd;

    // corresponding transmitter
    TocinoTx * const m_tx;

    std::vector< Ptr <CallbackQueue> > m_queues; // packet queues to write
    
    Ptr<TocinoRouter> m_router;
};

} // namespace ns3

#endif // __TOCINO_RX_H__
