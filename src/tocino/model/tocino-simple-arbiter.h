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

    uint32_t Arbitrate();

    void Initialize( Ptr<TocinoNetDevice>, const TocinoTx* );
    uint32_t GetVCOwner(uint32_t vc);

    static const uint32_t ANY_PORT;

private:
    Ptr<TocinoNetDevice> m_tnd;
    const TocinoTx *m_ttx;

    typedef std::vector<uint32_t> QueueVector;
    QueueVector BuildCandidateSet() const;
    uint32_t FairSelectWinner( const QueueVector& ) const;
    void UpdateState( uint32_t winner );

    typedef std::vector<uint32_t> PortVector;
    PortVector m_legalPort;
    
#ifdef TOCINO_VC_STRESS_MODE
    mutable uint8_t m_lastVC;
#endif
};

}
#endif //__TOCINO_SIMPLE_ARBITER_H__
