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

    TocinoArbiterAllocation Arbitrate();

    void Initialize( const TocinoNetDevice*, const TocinoTx* );
    
    TocinoArbiterAllocation GetVCOwner( const TocinoOutputVC ) const;

    static const TocinoArbiterAllocation ANY_QUEUE;

    private:

    typedef std::vector< TocinoArbiterAllocation > AllocVector;

    AllocVector BuildCandidateSet() const;

    TocinoArbiterAllocation FairSelectWinner( const AllocVector& ) const;

    void UpdateState( const TocinoArbiterAllocation );
    
    const TocinoNetDevice* m_tnd;
    const TocinoTx *m_ttx;

    // This nested class controls access to our
    // primary state variable
    class 
    {
        private:

        AllocVector vec;

        public:
        
        // If you're thinking about adding another 
        // friend function here, you're wrong. -MAS
        
        friend void
            TocinoSimpleArbiter::Initialize(
                const TocinoNetDevice*, const TocinoTx* );
        
        friend void
            TocinoSimpleArbiter::UpdateState(
                const TocinoArbiterAllocation );

        friend TocinoArbiterAllocation
            TocinoSimpleArbiter::GetVCOwner( const TocinoOutputVC ) const;

    }
    m_legalQueue;

#ifdef TOCINO_VC_STRESS_MODE
    mutable TocinoOutputVC m_lastVC;
#endif
};

}
#endif //__TOCINO_SIMPLE_ARBITER_H__
