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

    TocinoRoute Route( const TocinoInputVC );

    TocinoRoute GetCurrentRoute( const TocinoInputVC ) const;

    private:

    TocinoRoute ComputeNewRoute( Ptr<const Packet> ) const;

    bool NewRouteIsLegal( const TocinoRoute ) const;

    bool TopologyHasWrapAround() const;

    enum Direction { DIR_POS, DIR_NEG, DIR_INVALID };

    Direction DetermineRoutingDirection(
            const TocinoAddress::Coordinate, 
            const TocinoAddress::Coordinate ) const;

    bool RouteCrossesDateline(
            const TocinoAddress::Coordinate,
            const Direction ) const;
    
    bool RouteChangesDimension( const TocinoOutputPort ) const;

    bool TransmitterCanAcceptFlit(
            const TocinoOutputPort,
            const TocinoOutputVC ) const;

    void SetRoute( const TocinoInputVC, const TocinoRoute );

    Ptr<TocinoNetDevice> m_tnd;
    const TocinoRx *m_trx;

    static const TocinoAddress::Coordinate NO_WRAP = 0;
    TocinoAddress::Coordinate m_wrapPoint;

    // This nested class controls access to our
    // primary state variable
    class
    {
        private:
        
        std::vector< TocinoRoute > vec;

        public:
        
        // If you're thinking about adding another 
        // friend function here, you're wrong. -MAS
        
        friend void
            TocinoDimensionOrderRouter::Initialize(
                Ptr<TocinoNetDevice>, const TocinoRx* );
        
        friend TocinoRoute
            TocinoDimensionOrderRouter::GetCurrentRoute(
                const TocinoInputVC ) const;

        friend void 
            TocinoDimensionOrderRouter::SetRoute(
                const TocinoInputVC, const TocinoRoute );
    } 
    m_currentRoutes;
};

}

#endif //__TOCINO_DIMENSION_ORDER_ROUTER_H__
