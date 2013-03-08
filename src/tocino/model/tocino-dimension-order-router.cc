/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"

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
        .AddAttribute( "EnableWrapAround", 
            "The radix of rings/tori with wrap-around links.",
            UintegerValue( 0 ),
            MakeUintegerAccessor( &TocinoDimensionOrderRouter::EnableWrapAround ),
            MakeUintegerChecker<uint32_t>() )
        .AddAttribute( "OutOfOrderOK", 
            "It's OK to route packets in such a way that they may arrive out of order.",
            BooleanValue( false ),
            MakeBooleanAccessor( &TocinoDimensionOrderRouter::m_outOfOrderOK ),
            MakeBooleanChecker() )
        .AddConstructor<TocinoDimensionOrderRouter>();
    return tid;
}

TocinoDimensionOrderRouter::TocinoDimensionOrderRouter()
    : m_tnd( NULL )
    , m_inputPort( TOCINO_INVALID_PORT )
    , m_wrap( false )
    , m_radix( 0 )
{}

void 
TocinoDimensionOrderRouter::Initialize( 
        const TocinoNetDevice* tnd, 
        const TocinoInputPort inputPort )
{
    m_tnd = tnd;
    m_inputPort = inputPort;
}

bool TocinoDimensionOrderRouter::TopologyHasWrapAround() const
{
    return m_wrap;
}

TocinoDirection
TocinoDimensionOrderRouter::DetermineRoutingDirection(
        const TocinoAddress::Coordinate src,
        const TocinoAddress::Coordinate dst ) const
{
    const int32_t delta = dst - src;

    NS_ASSERT( delta != 0 );

    bool routePositive = delta > 0;
    
    if( TopologyHasWrapAround() )
    {
        if( abs(delta) == m_radix/2 )
        {
            // Tie-breaker
            if( m_outOfOrderOK )
            {
                // Alternate packet direction (may result in OoO delivery)
                static bool flip = false;
                    
                routePositive = flip;

                flip = !flip;
            }
            else 
            {
                // Even coords route positive, odds negative 
                routePositive = ( src & 1 );
            }
        }
        else if( abs(delta) > m_radix/2 )
        {
            routePositive = !routePositive;
        }
    }

    if( routePositive )
        return TOCINO_DIRECTION_POS;

    return TOCINO_DIRECTION_NEG;
}

bool
TocinoDimensionOrderRouter::RouteCrossesDateline(
        const TocinoAddress::Coordinate srcCoord,
        const TocinoDirection dir ) const
{
    NS_ASSERT( TopologyHasWrapAround() );

    if( (srcCoord == 0) && (dir == TOCINO_DIRECTION_NEG) )
        return true;

    if( (srcCoord == m_radix-1) && (dir == TOCINO_DIRECTION_POS) )
        return true;

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
 
    const bool injecting =
        m_inputPort == m_tnd->GetHostPort() ? true : false;
    
    const TocinoDimension inputDim = TocinoGetDimension( m_inputPort );
    
    TocinoDimension outputDim = TOCINO_INVALID_DIMENSION;
    TocinoDirection outputDir = TOCINO_INVALID_DIRECTION;

    TocinoAddress::Coordinate localCoord;
    TocinoAddress::Coordinate destCoord;

    // Dimension-order routing
    for( outputDim = TOCINO_DIMENSION_X;
            outputDim < TocinoAddress::MAX_DIM; ++outputDim )
    {
        localCoord = localAddr.GetCoordinate( outputDim );
        destCoord = destAddr.GetCoordinate( outputDim );

        if( localCoord != destCoord )
        {
            break;
        }
    }

    if( injecting || (inputDim != outputDim) )
    {
        // Changing dimension, choose new direction
        outputDir = DetermineRoutingDirection( localCoord, destCoord );
    }
    else
    {
        const TocinoDirection inputDir = 
            TocinoGetOppositeDirection( TocinoGetDirection( m_inputPort ) );

        // Continue in the same direction
        outputDir = inputDir;
    }
    
    NS_ASSERT( outputDim != TOCINO_INVALID_DIMENSION );
    NS_ASSERT( outputDir != TOCINO_INVALID_DIRECTION );
    
    outputPort = TocinoGetPort( outputDim, outputDir );

    NS_ASSERT( outputPort != TOCINO_INVALID_PORT );

    // Dateline algorithm for deadlock avoidance in rings/tori
    if( TopologyHasWrapAround() )
    {
        // Are we changing dimension?
        if( !injecting && (inputDim != outputDim) )
        {
            // Reset to the base (even-numbered) VC of the pair
            outputVC = inputVC.AsUInt32() & ~1;
        }
        else if( RouteCrossesDateline( localCoord, outputDir ) )
        {
            NS_ASSERT_MSG( inputVC < m_tnd->GetNVCs(), 
                    "Flit on last VC cannot cross dateline!" );

            // Switch to the high (odd-numbered) VC of the pair
            outputVC = inputVC.AsUInt32() + 1;
        }
    }

    NS_ASSERT( outputVC < m_tnd->GetNVCs() );

    return TocinoRoute( outputPort, inputVC, outputVC );
}

void 
TocinoDimensionOrderRouter::EnableWrapAround( uint32_t radix )
{
    if( radix == 0 ) return;

    m_radix = radix;
    m_wrap = true;
}

}
