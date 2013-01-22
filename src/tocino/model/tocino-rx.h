/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_RX_H__
#define __TOCINO_RX_H__

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"

#include "tocino-crossbar.h"
#include "tocino-flow-control.h"
#include "tocino-queue.h"
#include "tocino-router.h"
#include "tocino-routing-table.h"

namespace ns3
{

class NetDevice;
class TocinoChannel;
class TocinoTx;

class TocinoRx
{
    public:

    TocinoRx( const uint32_t, Ptr<TocinoNetDevice> );

    uint32_t GetPortNumber() const;

    Ptr<NetDevice> GetNetDevice();
    
    void SetChannel( Ptr<TocinoChannel> channel );

    bool IsVCBlocked( const TocinoInputVC ) const;

    void Receive(Ptr<Packet> p);
    
    void TryForwardFlit();

    bool AllQuiet() const;
    void DumpState() const;

    private:
  
    void AnnounceRoutingDecision(
            Ptr<const Packet>,
            const TocinoRoute& ) const;

    struct InputQueueEntry
    {
        Ptr<Packet> flit;
        TocinoRoute route;
    };

    typedef TocinoQueue< InputQueueEntry  > InputQueue;

    InputQueue& GetInputQueue( const TocinoInputVC );
    const InputQueue& GetInputQueue( const TocinoInputVC ) const;
    
    void SetReserveFlits( uint32_t );
    
    bool EnqueueHelper(
            const InputQueueEntry&,  
            const TocinoInputVC );
    
    // FIXME: Out parameters are ugly. Replace this with
    // std::tuple and std::tie ASAP so we can return
    // both the packet and the boolean at once.
    const InputQueueEntry DequeueHelper( 
            const TocinoInputVC,
            bool& );
    
    void RewriteFlitHeaderVC(
            Ptr<Packet>,
            const TocinoOutputVC ) const;
   
    TocinoInputVC FindForwardableVC() const;
       
    static const TocinoInputVC NO_FORWARDABLE_VC;

    const TocinoInputPort m_inputPort;

    const Ptr<TocinoNetDevice> m_tnd;
    Ptr<TocinoChannel> m_channel;

    // corresponding transmitter
    TocinoTx * const m_tx;
   
    Ptr<TocinoRouter> m_router;
    TocinoRoutingTable m_routingTable;
    TocinoCrossbar m_crossbar;

    // This nested class controls access to our
    // primary state variable
    class TocinoInputQueues
    {
        private:
        
        // The input queues are per input virtual channel.
        // These are used mainly for correctness: to
        // tolerate XOFF delay.
        std::vector< InputQueue > vec;

        public:
     
        // If you're thinking about adding another 
        // friend function here, you're wrong. -MAS
        
        friend 
            TocinoRx::TocinoRx( const uint32_t,
                Ptr<TocinoNetDevice> );
        
        friend InputQueue&
            TocinoRx::GetInputQueue(
                const TocinoInputVC ); 
        
        friend const InputQueue&
            TocinoRx::GetInputQueue(
                const TocinoInputVC ) const;
    } 
    m_inputQueues;

};

} // namespace ns3

#endif // __TOCINO_RX_H__
