/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ROUTER_H__
#define __TOCINO_ROUTER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

#include "tocino-misc.h"

namespace ns3
{

class Packet;
class TocinoNetDevice;
class TocinoRx;

struct TocinoRouter : public Object
{
    static TypeId GetTypeId( void );

    virtual TocinoQueueDescriptor Route( Ptr<const Packet> p ) = 0;

    virtual void Initialize( Ptr<TocinoNetDevice>, const TocinoRx* ) = 0;
};

}
#endif //__TOCINO_ROUTER_H__
