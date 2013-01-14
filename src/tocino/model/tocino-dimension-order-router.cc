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
                << m_trx->GetPortNumber() << " "; }
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
    , m_trx( NULL )
    , m_wrapPoint( NO_WRAP )
{}

void TocinoDimensionOrderRouter::Initialize( Ptr<TocinoNetDevice> tnd, const TocinoRx* trx )
{
    m_tnd = tnd;
    m_trx = trx;
    m_currentRoutes.assign( m_tnd->GetNVCs(), TOCINO_INVALID_QUEUE );
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
        return POS;

    return NEG;
}

bool
TocinoDimensionOrderRouter::RouteCrossesDateline(
        const TocinoAddress::Coordinate srcCoord,
        const Direction dir ) const
{
    NS_ASSERT( TopologyHasWrapAround() );

    if( (srcCoord == 0) && (dir == NEG) )
        return true;

    if( (srcCoord == m_wrapPoint) && (dir == POS) )
        return true;

    return false;
}

bool 
TocinoDimensionOrderRouter::RouteChangesDimension( const uint32_t outputPort ) const
{
    const uint32_t inputPort = m_trx->GetPortNumber();

    // ISSUE-REVIEW: what if outputPort == host port?
    if( inputPort == m_tnd->GetHostPort() )
    {
        return false;
    }

    if( (inputPort/2) != (outputPort/2) )
    {
        return true;
    }

    return false;
}

bool
TocinoDimensionOrderRouter::TransmitterCanAcceptFlit(
        const uint32_t outputPort,
        const uint32_t outputVC ) const
{
    TocinoTx* outputTransmitter = m_tnd->GetTransmitter( outputPort );
    const uint32_t inputPort = m_trx->GetPortNumber();

    if( outputTransmitter->CanAcceptFlit( inputPort, outputVC ) )
    {
        return true;
    }

    return false;
}

bool
TocinoDimensionOrderRouter::NewRouteIsLegal( const TocinoQueueDescriptor qd ) const
{
    // We must not have an existing route to this output queue, 
    // as this would result in our interleaving flits from different
    // packets into a single output queue.

    for( uint32_t inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        if( m_currentRoutes[inputVC] == qd )
        {
            return false;
        }
    }

    return true;
}

TocinoQueueDescriptor
TocinoDimensionOrderRouter::ComputeNewRoute( Ptr<const Packet> flit ) const
{
    NS_LOG_FUNCTION( GetTocinoFlitIdString( flit ) );
    NS_ASSERT( IsTocinoFlitHead( flit ) );
    
    const uint32_t inputVC = GetTocinoFlitVirtualChannel( flit );

    TocinoAddress localAddr = m_tnd->GetTocinoAddress();
    TocinoAddress destAddr = GetTocinoFlitDestination( flit );

    if( destAddr == localAddr )
    {
        // Deliver successfully-routed flit to host
        const uint32_t hostPort = m_tnd->GetHostPort();
        return TocinoQueueDescriptor( hostPort, inputVC );
    }

    // Default assumption is that we do not switch VCs
    uint32_t outputVC = inputVC;
    uint32_t outputPort = TOCINO_INVALID_PORT;

    // Dimension-order routing
    Direction dir;

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

            if( dir == POS )
            {
                outputPort = PORT_POS;
            }
            else
            {
                NS_ASSERT( dir == NEG );
                outputPort = PORT_NEG;
            }

            break;
        }
    }

    NS_ASSERT( outputPort != TOCINO_INVALID_PORT );

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
            outputVC = inputVC + 1;
        }
    }

    return TocinoQueueDescriptor( outputPort, outputVC );
}

TocinoQueueDescriptor
TocinoDimensionOrderRouter::Route( const uint32_t inputVC ) 
{
    NS_LOG_FUNCTION_NOARGS();

    NS_ASSERT( m_tnd != NULL );

    Ptr<const Packet> flit = m_trx->PeekNextFlit( inputVC ); 

    NS_ASSERT( flit != NULL );

    const bool isHead = IsTocinoFlitHead( flit );
    const bool isTail = IsTocinoFlitTail( flit );

    std::ostringstream logPrefix;

    TocinoQueueDescriptor route( TOCINO_INVALID_QUEUE );

    if( isHead )
    {
        NS_ASSERT( m_currentRoutes[inputVC] == TOCINO_INVALID_QUEUE );

        // Make a new routing decision
        TocinoQueueDescriptor newRoute = ComputeNewRoute( flit );

        if( NewRouteIsLegal( newRoute ) )
        {
            logPrefix << "establishing new route ";
            route = newRoute;
        }
        else
        {
            const uint32_t outputPort = newRoute.GetPort();
            const uint32_t outputVC = newRoute.GetVirtualChannel();

            NS_LOG_LOGIC( "route in progress to outputPort="
                    << outputPort << ", outputVC=" << outputVC );

            return CANNOT_ROUTE;
        }
    }
    else
    {
        NS_ASSERT( m_currentRoutes[inputVC] != TOCINO_INVALID_QUEUE );

        // Recall previous routing decision
        route = m_currentRoutes[inputVC];

        logPrefix << "using existing route ";
    }

    NS_ASSERT( route != TOCINO_INVALID_QUEUE );

    const uint32_t outputPort = route.GetPort();
    const uint32_t outputVC = route.GetVirtualChannel();

    logPrefix << "via " << Tocino3dTorusPortNumberToString( outputPort );

    if( !TransmitterCanAcceptFlit( outputPort, outputVC ) )
    {
        return CANNOT_ROUTE;
    }

    if( inputVC == outputVC )
    {
        NS_LOG_LOGIC( logPrefix.str() << " over VC " << outputVC );
    }
    else
    {
        NS_LOG_LOGIC( logPrefix.str() << " from input VC " << inputVC 
                << " to output VC " << outputVC );
    }

    if( isHead && !isTail )
    {
        // Record new route 
        m_currentRoutes[inputVC] = route;
    }
    else if( !isHead && isTail )
    {
        // Tear down routing decision by resetting state on a tail flit.
        // (So called "head-tail" flits must also execute this code.)
        NS_LOG_LOGIC( "tail flit, clearing state for input VC "
                << (uint32_t) inputVC );

        m_currentRoutes[inputVC] = TOCINO_INVALID_QUEUE;
    }

    // Return routing decision
    return route;
}

    
TocinoQueueDescriptor
TocinoDimensionOrderRouter::GetCurrentRoute( uint8_t inputVC ) const
{
    NS_ASSERT( inputVC < m_currentRoutes.size() );
    return m_currentRoutes[inputVC];
}

}
