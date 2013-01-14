/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ROUTER_H__
#define __TOCINO_ROUTER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

#include "tocino-queue-descriptor.h"

namespace ns3
{

class Packet;
class TocinoNetDevice;
class TocinoRx;

struct TocinoRouter : public Object
{
    static TypeId GetTypeId( void );

    virtual TocinoQueueDescriptor Route( const uint32_t ) = 0;

    virtual TocinoQueueDescriptor GetCurrentRoute( uint8_t ) const = 0;

    virtual void Initialize( Ptr<TocinoNetDevice>, const TocinoRx* ) = 0;
    
    static const TocinoQueueDescriptor CANNOT_ROUTE;
};

}
#endif //__TOCINO_ROUTER_H__
