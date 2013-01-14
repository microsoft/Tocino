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

    uint32_t GetPortNumber() const;

    Ptr<NetDevice> GetNetDevice();
    
    bool IsQueueBlocked( const uint32_t, const uint32_t, const uint32_t ) const;
    bool IsAnyQueueBlocked() const ;
    bool IsVCBlocked( uint32_t ) const;

    Ptr<const Packet> PeekNextFlit( const uint32_t ) const;
    bool CanRouteFrom( uint32_t ) const;

    void RewriteFlitHeaderVC( Ptr<Packet>, const uint32_t ) const;
    
    void Receive(Ptr<Packet> p);
    
    void TryRouteFlit();

    void SetReserveFlits( uint32_t );
    
    bool AllQuiet() const;
    void DumpState() const;

private:
   
    bool EnqueueHelper( Ptr<Packet>, const uint32_t );
    
    // FIXME: Out parameters are ugly. Replace this with
    // std::tuple and std::tie ASAP so we can return
    // both the packet and the boolean at once.
    Ptr<Packet> DequeueHelper( const uint32_t, bool& );

    const uint32_t m_inputPortNumber;

    // link to owning TocinoNetDevice
    const Ptr<TocinoNetDevice> m_tnd;

    // corresponding transmitter
    TocinoTx * const m_tx;

    // The inputQueues are per input virtual channel.
    // These are used mainly for correctness: to
    // tolerate XOFF delay.
    std::vector< Ptr<CallbackQueue> > m_inputQueues;
    
    Ptr<TocinoRouter> m_router;
};

} // namespace ns3

#endif // __TOCINO_RX_H__
