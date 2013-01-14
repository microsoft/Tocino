/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ARBITER_H__
#define __TOCINO_ARBITER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

#include "tocino-queue-descriptor.h"

namespace ns3
{

class TocinoNetDevice;
class TocinoTx;

struct TocinoArbiter : public Object
{
    static TypeId GetTypeId( void );

    virtual TocinoQueueDescriptor Arbitrate() = 0;

    virtual void Initialize( Ptr<TocinoNetDevice>, const TocinoTx* ) = 0;

    virtual TocinoQueueDescriptor GetVCOwner( const uint32_t ) const = 0;

    static const TocinoQueueDescriptor DO_NOTHING;
};

}
#endif //__TOCINO_ARBITER_H__
