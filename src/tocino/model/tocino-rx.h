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
    
    bool IsQueueBlocked( const uint32_t, const uint8_t, const uint8_t ) const;
    bool IsAnyQueueBlocked() const ;
    bool IsVCBlocked( uint8_t ) const;

    void SetQueue( uint32_t, uint8_t, uint8_t, Ptr<CallbackQueue> );
   
    bool RouteChangesVC( Ptr<const Packet>, const uint8_t ) const;
    void RewriteFlitHeaderVC( Ptr<Packet>, const uint8_t ) const;
    
    void Receive(Ptr<Packet> p);
   
    void SetReserveFlits( uint32_t );

    void DumpState();

private:
   
    bool EnqueueHelper( Ptr<Packet>, const TocinoQueueDescriptor );

    const uint32_t m_portNumber;

    // link to owning TocinoNetDevice
    const Ptr<TocinoNetDevice> m_tnd;

    // corresponding transmitter
    TocinoTx * const m_tx;

    typedef std::vector< Ptr<CallbackQueue> > OutputVCVec;
    typedef std::vector< OutputVCVec > InputVCVec;
    typedef std::vector< InputVCVec > RxQueueVec;

    RxQueueVec m_queues;
    
    Ptr<TocinoRouter> m_router;
};

} // namespace ns3

#endif // __TOCINO_RX_H__
