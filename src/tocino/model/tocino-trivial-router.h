/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TRIVIAL_ROUTER_H__
#define __TOCINO_TRIVIAL_ROUTER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

#include "tocino-router.h"

namespace ns3
{

class Packet;

class TocinoTrivialRouter : public TocinoRouter
{
    public:
    
    static TypeId GetTypeId( void );
    TocinoTrivialRouter();
    void Initialize( Ptr<TocinoNetDevice> tnd );
    uint32_t Route( const uint32_t, Ptr<const Packet> );

    private:

    Ptr<TocinoNetDevice> m_tnd;
};

}
#endif //__TOCINO_TRIVIAL_ROUTER_H__
