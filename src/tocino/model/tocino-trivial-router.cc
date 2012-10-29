/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-trivial-router.h"

#include "ns3/log.h"
#include "ns3/packet.h"

#include "tocino-flit-header.h"
#include "tocino-net-device.h"

NS_LOG_COMPONENT_DEFINE ("TocinoTrivialRouter");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (TocinoTrivialRouter);

TypeId TocinoTrivialRouter::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoTrivialRouter" )
        .SetParent<TocinoRouter>()
        .AddConstructor<TocinoTrivialRouter>();
    return tid;
}

TocinoTrivialRouter::TocinoTrivialRouter()
    : m_tnd( NULL )
{}

void TocinoTrivialRouter::Initialize( Ptr<TocinoNetDevice> tnd )
{
    m_tnd = tnd;
}

uint32_t
TocinoTrivialRouter::Route( const uint32_t, Ptr<const Packet> f )
{
    NS_ASSERT( m_tnd != NULL );

    TocinoFlitHeader h;
    f->PeekHeader( h );
  
    uint32_t port = 0;

    if( h.GetDestination() == m_tnd->GetAddress() )
    {
        port = m_tnd->GetHostPort();
    }

    return m_tnd->PortToQueue( port );
} 

}
