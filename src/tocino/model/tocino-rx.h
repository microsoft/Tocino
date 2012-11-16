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

class TocinoRx
{
public:
    TocinoRx( const uint32_t, Ptr<TocinoNetDevice>, Ptr<TocinoRouter> );
    ~TocinoRx();
    
    Ptr<NetDevice> GetNetDevice();
    
    void SetXState(TocinoFlowControl::State s);
    TocinoFlowControl::State GetXState() const;
    
    bool IsQueueBlocked( uint32_t ) const;
    bool IsAnyQueueBlocked() const ;
    bool IsVCBlocked( uint8_t ) const;

    void CheckForUnblock(); // called from TocinNetDeviceTransmitter

    void SetQueue( uint32_t, Ptr<CallbackQueue> );
    
    void Receive(Ptr<Packet> p);
    
private:
    
    const uint32_t m_portNumber;

    TocinoFlowControl::State m_xstate; // tracks xstate of TocinoTx on other end of channel

    const Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
    
    std::vector< Ptr <CallbackQueue> > m_queues; // packet queues to write
    
    Ptr<TocinoRouter> m_router;
};

} // namespace ns3

#endif // __TOCINO_RX_H__
