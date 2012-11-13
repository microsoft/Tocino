/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_SIMPLE_ARBITER_H__
#define __TOCINO_SIMPLE_ARBITER_H__   

#include "tocino-arbiter.h"
#include "tocino-net-device.h"

namespace ns3
{

//class Packet;
class TocinoNetDevice;

class TocinoSimpleArbiter : public TocinoArbiter
{
    public:
    static TypeId GetTypeId( void );
    
    TocinoSimpleArbiter();

    //virtual uint32_t Arbitrate( const uint32_t inPort, Ptr<const Packet> p ) = 0;
    int Arbitrate();

    void Initialize( Ptr<TocinoNetDevice>, const TocinoTx* );
   
    private:
    Ptr<TocinoNetDevice> m_tnd;
    const TocinoTx *m_ttx;
};

}
#endif //__TOCINO_SIMPLE_ARBITER_H__
