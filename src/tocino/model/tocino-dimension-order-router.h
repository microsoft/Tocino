/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_DIMENSION_ORDER_ROUTER_H__
#define __TOCINO_DIMENSION_ORDER_ROUTER_H__

#include <vector>

#include "tocino-router.h"
#include "tocino-net-device.h"
#include "tocino-address.h"
#include "tocino-misc.h"

namespace ns3
{

class TocinoDimensionOrderRouter : public TocinoRouter
{
    public:

    static TypeId GetTypeId( void );

    TocinoDimensionOrderRouter();

    void Initialize( const TocinoNetDevice*, const TocinoInputPort );

    TocinoRoute Route( Ptr<const Packet> ) const;

    void EnableWrapAround( uint32_t );
    
    private:

    bool TopologyHasWrapAround() const;

    TocinoDirection DetermineRoutingDirection(
            const TocinoAddress::Coordinate, 
            const TocinoAddress::Coordinate ) const;

    bool RouteCrossesDateline(
            const TocinoAddress::Coordinate,
            const TocinoDirection ) const;

    const TocinoNetDevice* m_tnd;
    TocinoInputPort m_inputPort;

    bool m_wrap;
    uint32_t m_radix;
};

}

#endif //__TOCINO_DIMENSION_ORDER_ROUTER_H__
