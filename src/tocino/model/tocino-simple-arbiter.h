/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_SIMPLE_ARBITER_H__
#define __TOCINO_SIMPLE_ARBITER_H__   

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

    private:
    Ptr<TocinoNetDevice> m_tnd;
    const TocinoTx *m_ttx;

    bool AllQueuesEmpty() const;
    uint32_t FairSelectWinner() const;

    //FIXME need entry for each VC
    uint32_t m_currentWinner;
};

}
#endif //__TOCINO_SIMPLE_ARBITER_H__
