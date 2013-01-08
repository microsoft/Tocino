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

    void Initialize( Ptr<TocinoNetDevice>, const TocinoRx* );

    TocinoQueueDescriptor Route( Ptr<const Packet> p );

    private:

    bool TopologyHasWrapAround() const;

    enum Direction { POS, NEG };
    Direction DetermineRoutingDirection(
            const TocinoAddress::Coordinate, 
            const TocinoAddress::Coordinate ) const;

    Ptr<TocinoNetDevice> m_tnd;
    const TocinoRx *m_trx;

    static const TocinoAddress::Coordinate NO_WRAP = 0;
    TocinoAddress::Coordinate m_wrapPoint;

    std::vector< TocinoQueueDescriptor > m_currentRoutes;
};

}

#endif //__TOCINO_DIMENSION_ORDER_ROUTER_H__
