/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#include "tocino-dimension-order-router.h"
#include "tocino-misc.h"
#include "tocino-flit-header.h"
#include "tocino-rx.h"

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
    m_currentRoutes.assign( m_tnd->GetNVCs(), TOCINO_INVALID_PORT );
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

uint32_t TocinoDimensionOrderRouter::Route( Ptr<const Packet> p ) 
{
    NS_ASSERT( m_tnd != NULL );

    TocinoFlitHeader h;
    p->PeekHeader( h );

    int vc = h.GetVirtualChannel();

    int outputPort = m_currentRoutes[vc];

    if( h.IsHead() )
    {
        NS_ASSERT( m_currentRoutes[vc] == TOCINO_INVALID_PORT );

        TocinoAddress localAddr = m_tnd->GetTocinoAddress();
        TocinoAddress destAddr = h.GetDestination();
       
        if( destAddr == localAddr )
        {
            // deliver successfully-routed flit to host
            outputPort = m_tnd->GetHostPort(); 
        }
        else
        {
            // dimension-order routing
            for( int dim = 0; dim < TocinoAddress::DIM; ++dim )
            {
                TocinoAddress::Coordinate lc = localAddr.GetCoordinate(dim);
                TocinoAddress::Coordinate dc = destAddr.GetCoordinate(dim);

                if( lc != dc )
                {
                    // FIXME: router should not be assuming this
                    const int PORT_POS = dim*2;
                    const int PORT_NEG = PORT_POS+1;

                    Direction dir = DetermineRoutingDirection( lc, dc );

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
        }
        
        NS_LOG_LOGIC( "routing to "
            << Tocino3dTorusPortNumberToString( outputPort ) );

        m_currentRoutes[vc] = outputPort;
    }
    
    NS_ASSERT( m_currentRoutes[vc] != TOCINO_INVALID_PORT );

    if( h.IsTail() )
    {
        m_currentRoutes[vc] = TOCINO_INVALID_PORT;
        //NS_LOG_LOGIC("removing established path");
    }

    //FIXME always stay on same VC for now
    return m_tnd->PortToQueue( outputPort, vc ); 
}

}
