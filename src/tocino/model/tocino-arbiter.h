/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ARBITER_H__
#define __TOCINO_ARBITER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

namespace ns3
{

class TocinoNetDevice;
class TocinoTx;

struct TocinoArbiter : public Object
{
    static TypeId GetTypeId( void );

    virtual int Arbitrate() = 0;

    virtual void Initialize( Ptr<TocinoNetDevice>, const TocinoTx* ) = 0;
};

}
#endif //__TOCINO_ARBITER_H__
