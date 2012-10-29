/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/log.h"

#include "tocino-dimension-order-router.h"
#include "tocino-misc.h"
#include "tocino-flit-header.h"

NS_LOG_COMPONENT_DEFINE ("TocinoDimensionOrderRouter");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (TocinoDimensionOrderRouter);

TypeId TocinoDimensionOrderRouter::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoDimensionOrderRouter" )
        .SetParent<TocinoRouter>()
        .AddConstructor<TocinoDimensionOrderRouter>();
    return tid;
}

TocinoDimensionOrderRouter::TocinoDimensionOrderRouter()
    : m_tnd( NULL )
{}

void TocinoDimensionOrderRouter::Initialize( Ptr<TocinoNetDevice> tnd )
{
    m_tnd = tnd;
    m_routeVector.assign( m_tnd->GetNPorts(), TOCINO_INVALID_PORT );
}

uint32_t TocinoDimensionOrderRouter::Route( const uint32_t inPort, Ptr<const Packet> p ) 
{
    NS_ASSERT( m_tnd != NULL );

    TocinoFlitHeader h;
    p->PeekHeader( h );

    int outputPort = m_routeVector[inPort];

    if( h.IsHead() )
    {
        NS_ASSERT( m_routeVector[inPort] == TOCINO_INVALID_PORT );

        TocinoAddress localAddr =
            TocinoAddress::ConvertFrom( m_tnd->GetAddress() );

        uint8_t x = localAddr.GetX();
        uint8_t dx = h.GetDestination().GetX();

        uint8_t y = localAddr.GetY();
        uint8_t dy = h.GetDestination().GetY();

        uint8_t z = localAddr.GetZ();
        uint8_t dz = h.GetDestination().GetZ();

        // dimension-order routing
        
        if( dx > x ) 
        {
            //NS_LOG_LOGIC("routing to 0/x+");
            outputPort = 0; // x+
        }
        else if( dx < x )
        {
            //NS_LOG_LOGIC("routing to 1/x-");
            outputPort = 1; // x-
        }
        else if( dy > y )
        {
            //NS_LOG_LOGIC("routing to 2/y+");
            outputPort = 2; // y+
        }
        else if( dy < y )
        {
            //NS_LOG_LOGIC("routing to 3/y-");
            outputPort = 3; // y-
        }
        else if( dz > z )
        {
            //NS_LOG_LOGIC("routing to 4/z+");
            outputPort = 4; // z+
        }
        else if( dz < z )
        {
            //NS_LOG_LOGIC("routing to 5/z-");
            outputPort = 5; // z-
        }
        else
        {
            // deliver successfully-routed flit to host
            NS_ASSERT( h.GetDestination() == m_tnd->GetAddress() );
            
            //NS_LOG_LOGIC("routing to 6/ejection");
            outputPort = m_tnd->GetHostPort(); 
        }

        m_routeVector[inPort] = outputPort;
    }
    
    NS_ASSERT( m_routeVector[inPort] != TOCINO_INVALID_PORT );

    if( h.IsTail() )
    {
        m_routeVector[inPort] = TOCINO_INVALID_PORT;
        //NS_LOG_LOGIC("removing established path");
    }

    //FIXME always to vc 0 for now
    return m_tnd->PortToQueue( outputPort ); 
}

}
