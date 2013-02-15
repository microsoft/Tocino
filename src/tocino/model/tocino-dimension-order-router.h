/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_DIMENSION_ORDER_ROUTER_H__
#define __TOCINO_DIMENSION_ORDER_ROUTER_H__

#include <vector>

#include "tocino-router.h"
#include "tocino-net-device.h"
#include "tocino-address.h"

namespace ns3
{

class TocinoDimensionOrderRouter : public TocinoRouter
{
    public:
    static TypeId GetTypeId( void );

    TocinoDimensionOrderRouter();

    void Initialize( const TocinoNetDevice*, const TocinoInputPort );

    TocinoRoute Route( Ptr<const Packet> ) const;

    private:

    bool TopologyHasWrapAround() const;

    enum Direction { DIR_POS, DIR_NEG, DIR_INVALID };

    Direction DetermineRoutingDirection(
            const TocinoAddress::Coordinate, 
            const TocinoAddress::Coordinate ) const;

    bool RouteCrossesDateline(
            const TocinoAddress::Coordinate,
            const Direction ) const;
    
    bool RouteChangesDimension( const TocinoOutputPort ) const;

    const TocinoNetDevice* m_tnd;
    TocinoInputPort m_inputPort;

    static const TocinoAddress::Coordinate NO_WRAP = 0;
    TocinoAddress::Coordinate m_wrapPoint;
};

}

#endif //__TOCINO_DIMENSION_ORDER_ROUTER_H__
