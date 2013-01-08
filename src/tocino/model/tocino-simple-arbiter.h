/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_SIMPLE_ARBITER_H__
#define __TOCINO_SIMPLE_ARBITER_H__   

#include <vector>

#include "tocino-arbiter.h"
#include "tocino-net-device.h"

namespace ns3
{

class TocinoNetDevice;

class TocinoSimpleArbiter : public TocinoArbiter
{
public:
    static TypeId GetTypeId( void );
    
    TocinoSimpleArbiter();

    TocinoQueueDescriptor Arbitrate();

    void Initialize( Ptr<TocinoNetDevice>, const TocinoTx* );
    
    uint32_t GetVCOwner( const uint8_t vc );

    static const uint32_t ANY_PORT;

private:
    Ptr<TocinoNetDevice> m_tnd;
    const TocinoTx *m_ttx;

    typedef std::vector< TocinoQueueDescriptor > QueueVector;

    QueueVector BuildCandidateSet() const;

    TocinoQueueDescriptor FairSelectWinner( const QueueVector& ) const;

    void UpdateState( const TocinoQueueDescriptor );

    typedef std::vector<uint32_t> PortVector;
    PortVector m_legalPort;
    
#ifdef TOCINO_VC_STRESS_MODE
    mutable uint8_t m_lastVC;
#endif
};

}
#endif //__TOCINO_SIMPLE_ARBITER_H__
