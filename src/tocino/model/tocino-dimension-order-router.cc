/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#include "tocino-dimension-order-router.h"
#include "tocino-misc.h"
#include "tocino-flit-header.h"
#include "tocino-rx.h"
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

// FIXME: should this be called RouteChangesDimension?
bool 
TocinoDimensionOrderRouter::RouteChangesDirection( const uint32_t outputPort ) const
{
    const uint32_t inputPort = m_trx->GetPortNumber();

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

TocinoQueueDescriptor
TocinoDimensionOrderRouter::Route( Ptr<const Packet> flit ) 
{
    NS_LOG_FUNCTION( GetTocinoFlitIdString( flit ) );

    NS_ASSERT( m_tnd != NULL );

    const uint8_t inputVC = GetTocinoFlitVirtualChannel( flit );

    uint32_t outputPort = TOCINO_INVALID_PORT;
    uint8_t outputVC = TOCINO_INVALID_VC;
        
    std::ostringstream logPrefix;

    if( IsTocinoFlitHead( flit ) )
    {
        // Make a new routing decision
        
        NS_ASSERT( m_currentRoutes[inputVC] == TOCINO_INVALID_QUEUE );
        
        // We typically remain on the input VC
        outputVC = inputVC;

        TocinoAddress localAddr = m_tnd->GetTocinoAddress();
        TocinoAddress destAddr = GetTocinoFlitDestination( flit );
       
        if( destAddr == localAddr )
        {
            // Deliver successfully-routed flit to host
            outputPort = m_tnd->GetHostPort(); 
        }
        else
        {
            // Dimension-order routing
            Direction dir;
            
            TocinoAddress::Coordinate localCoord;
            TocinoAddress::Coordinate destCoord;
        
            bool madeRoutingDecision = false;

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

                    madeRoutingDecision = true;
                    break;
                }
            }

            NS_ASSERT( madeRoutingDecision );
           
            // Default operation is not to switch VC
            outputVC = inputVC;
  
#if 1
            // Dateline algorithm for deadlock avoidance in rings/tori
            if( TopologyHasWrapAround() )
            {
                if( RouteChangesDirection( outputPort ) )
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
#endif
        }
        
        NS_ASSERT( outputPort != TOCINO_INVALID_PORT );
        NS_ASSERT( inputVC != TOCINO_INVALID_VC );
        NS_ASSERT( outputVC != TOCINO_INVALID_VC );

        TocinoQueueDescriptor outputQueue( outputPort, inputVC, outputVC );

        // Paranoia: 
        // we shouldn't have an existing route to this output queue
        for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
        {
            NS_ASSERT_MSG( m_currentRoutes[vc] != outputQueue,
                "cannot interleave flits from multiple packets in a single queue" );
        }
        
        // Store routing decision for future flits from the same packet
        m_currentRoutes[inputVC] = outputQueue;

        logPrefix << "establishing new route via "
            << Tocino3dTorusPortNumberToString( outputPort );
 
    }
    else
    {
        // Recall previous routing decision
        
        NS_ASSERT( m_currentRoutes[inputVC] != TOCINO_INVALID_QUEUE );

        outputPort = m_currentRoutes[inputVC].port;
        outputVC = m_currentRoutes[inputVC].outputVC;
        
        logPrefix << "using existing route via "
            << Tocino3dTorusPortNumberToString( outputPort );
    }
    
    if( inputVC == outputVC )
    {
        NS_LOG_LOGIC( logPrefix.str()
                << " over VC "
                << (uint32_t ) outputVC );
    }
    else
    {
        NS_LOG_LOGIC( logPrefix.str()
                << " from input VC "
                << (uint32_t ) inputVC 
                << " to output VC "
                << (uint32_t ) outputVC );
    }
    
    NS_ASSERT( m_currentRoutes[inputVC] != TOCINO_INVALID_QUEUE );

    // Tear down routing decision by resetting state on a tail flit.
    // (So called "head-tail" flits must also execute this code.)
    if( IsTocinoFlitTail( flit ) )    
    {
        NS_LOG_LOGIC( "tail flit, clearing state for input VC "
            << (uint32_t ) inputVC );

        m_currentRoutes[inputVC] = TOCINO_INVALID_QUEUE;
    }
    
    NS_ASSERT( outputPort != TOCINO_INVALID_PORT );
    NS_ASSERT( inputVC != TOCINO_INVALID_VC );
    NS_ASSERT( outputVC != TOCINO_INVALID_VC );
    
    return TocinoQueueDescriptor( outputPort, inputVC, outputVC );
}
    
TocinoQueueDescriptor
TocinoDimensionOrderRouter::GetCurrentRoute( uint8_t inputVC ) const
{
    NS_ASSERT( inputVC < m_currentRoutes.size() );
    return m_currentRoutes[inputVC];
}


}
