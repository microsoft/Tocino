/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ROUTER_H__
#define __TOCINO_ROUTER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

namespace ns3
{

class Packet;
class TocinoNetDevice;

struct TocinoRouter : public Object
{
    static TypeId GetTypeId( void );

    virtual uint32_t Route( Ptr<const Packet> p ) = 0;

    virtual void Initialize( Ptr<TocinoNetDevice> ) = 0;
};

}
#endif //__TOCINO_ROUTER_H__
