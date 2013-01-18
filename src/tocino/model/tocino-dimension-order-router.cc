/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#include "tocino-dimension-order-router.h"
#include "tocino-misc.h"
#include "tocino-flit-header.h"
#include "tocino-rx.h"
#include "tocino-tx.h"
#include "tocino-flit-id-tag.h"

NS_LOG_COMPONENT_DEFINE ("TocinoDimensionOrderRouter");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->GetTocinoAddress().GetX() << "," \
                << (int) m_tnd->GetTocinoAddress().GetY() << "," \
                << (int) m_tnd->GetTocinoAddress().GetZ() << ") " \
                << m_inputPort << " "; }
#endif

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (TocinoDimensionOrderRouter);

TypeId TocinoDimensionOrderRouter::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoDimensionOrderRouter" )
        .SetParent<TocinoRouter>()
        .AddAttribute ("WrapPoint", 
            "For rings/tori, the maximum coordinate value in any dimension",
            UintegerValue (TocinoDimensionOrderRouter::NO_WRAP),
            MakeUintegerAccessor (&TocinoDimensionOrderRouter::m_wrapPoint),
            MakeUintegerChecker<TocinoAddress::Coordinate> ())
        .AddConstructor<TocinoDimensionOrderRouter>();
    return tid;
}

TocinoDimensionOrderRouter::TocinoDimensionOrderRouter()
    : m_tnd( NULL )
    , m_inputPort( TOCINO_INVALID_PORT )
    , m_wrapPoint( NO_WRAP )
{}

void 
TocinoDimensionOrderRouter::Initialize( 
        Ptr<TocinoNetDevice> tnd, 
        const TocinoInputPort inputPort )
{
    m_tnd = tnd;
    m_inputPort = inputPort;
}

bool TocinoDimensionOrderRouter::TopologyHasWrapAround() const
{
    return m_wrapPoint != NO_WRAP;
}

TocinoDimensionOrderRouter::Direction
TocinoDimensionOrderRouter::DetermineRoutingDirection(
        const TocinoAddress::Coordinate src,
        const TocinoAddress::Coordinate dst ) const
{
    const int32_t delta = dst - src;

    NS_ASSERT( delta != 0 );

    bool routePositive = delta > 0;
    
    if( TopologyHasWrapAround() )
    {
        if( abs(delta) > m_wrapPoint/2 )
        {
            routePositive = !routePositive;
        }
    }

    if( routePositive )
        return DIR_POS;

    return DIR_NEG;
}

bool
TocinoDimensionOrderRouter::RouteCrossesDateline(
        const TocinoAddress::Coordinate srcCoord,
        const Direction dir ) const
{
    NS_ASSERT( TopologyHasWrapAround() );

    if( (srcCoord == 0) && (dir == DIR_NEG) )
        return true;

    if( (srcCoord == m_wrapPoint) && (dir == DIR_POS) )
        return true;

    return false;
}

bool 
TocinoDimensionOrderRouter::RouteChangesDimension(
        const TocinoOutputPort outputPort ) const
{
    // ISSUE-REVIEW: what if outputPort == host port?
    if( m_inputPort == m_tnd->GetHostPort() )
    {
        return false;
    }

    const uint32_t ip = m_inputPort.AsUInt32();
    const uint32_t op = outputPort.AsUInt32();

    if( (ip/2) != (op/2) )
    {
        return true;
    }

    return false;
}

TocinoRoute
TocinoDimensionOrderRouter::Route( Ptr<const Packet> flit ) const 
{
    NS_ASSERT( flit != NULL );
    
    NS_LOG_FUNCTION( GetTocinoFlitIdString( flit ) );
    NS_ASSERT( IsTocinoFlitHead( flit ) );
    
    const TocinoInputVC inputVC = GetTocinoFlitVirtualChannel( flit );
    
    // Default assumption is that we do not switch VCs
    TocinoOutputVC outputVC = inputVC.AsUInt32();
    
    TocinoOutputPort outputPort = TOCINO_INVALID_PORT;

    TocinoAddress localAddr = m_tnd->GetTocinoAddress();
    TocinoAddress destAddr = GetTocinoFlitDestination( flit );

    if( destAddr == localAddr )
    {
        // Deliver successfully-routed flit to host
        outputPort = m_tnd->GetHostPort();
        return TocinoRoute( outputPort, inputVC, outputVC );
    }
    
    // Dimension-order routing
    Direction dir = DIR_INVALID;

    TocinoAddress::Coordinate localCoord;
    TocinoAddress::Coordinate destCoord;

    for( int dim = 0; dim < TocinoAddress::DIM; ++dim )
    {
        localCoord = localAddr.GetCoordinate(dim);
        destCoord = destAddr.GetCoordinate(dim);

        if( localCoord != destCoord )
        {
            // FIXME: router should not be assuming this
            const int PORT_POS = dim*2;
            const int PORT_NEG = PORT_POS+1;

            dir = DetermineRoutingDirection( localCoord, destCoord );

            if( dir == DIR_POS )
            {
                outputPort = PORT_POS;
            }
            else
            {
                NS_ASSERT( dir == DIR_NEG );
                outputPort = PORT_NEG;
            }

            break;
        }
    }

    NS_ASSERT( outputPort != TOCINO_INVALID_PORT );
    NS_ASSERT( dir != DIR_INVALID );

    // Dateline algorithm for deadlock avoidance in rings/tori
    if( TopologyHasWrapAround() )
    {
        if( RouteChangesDimension( outputPort ) )
        {
            // Reset to virtual channel zero
            outputVC = 0;
        }
        else if( RouteCrossesDateline( localCoord, dir ) )
        {
            NS_ASSERT_MSG( inputVC < m_tnd->GetNVCs(), 
                    "Flit on last VC cannot cross dateline!" );

            // Switch to the next virtual channel
            outputVC = inputVC.AsUInt32() + 1;
        }
    }

    return TocinoRoute( outputPort, inputVC, outputVC );
}

}
