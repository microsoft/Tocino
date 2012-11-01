/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "test-tocino-deadlock.h"

#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"

#include "ns3/tocino-helper.h"
#include "ns3/tocino-channel.h"
#include "ns3/simulator.h"

using namespace ns3;

TestTocinoDeadlock::TestTocinoDeadlock()
  : TestCase( "Test dateline algorithm for deadlock avoidance" )
{
}

TestTocinoDeadlock::~TestTocinoDeadlock() {}

namespace
{
    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet> p, uint16_t, const Address& src )
    {
        //fprintf( stderr, "Got a flit\n" );
        return true;
    }
}

Ptr<TocinoNetDevice> TestTocinoDeadlock::CreateNetDeviceHelper( const TocinoAddress& a )
{
    Ptr<TocinoNetDevice> netDevice = CreateObject<TocinoNetDevice>();
    
    netDevice->Initialize();
    netDevice->SetAddress( a );
    netDevice->SetReceiveCallback( MakeCallback( AcceptPacket ) );

    // HACK: The Nodes are required to avoid
    // SIGSEGV in TocinoChannel::TransmitEnd()
    Ptr<Node> node = CreateObject<Node>();
    netDevice->SetNode( node );
    
    return netDevice;
}

namespace
{
    const TocinoAddress ADDR_A(0,0,0);
    const TocinoAddress ADDR_B(1,0,0);
    const TocinoAddress ADDR_C(1,1,0);
    const TocinoAddress ADDR_D(0,1,0);
}

void TestTocinoDeadlock::TestHelper( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
    
    Ptr<TocinoNetDevice> netDeviceA = CreateNetDeviceHelper( ADDR_A );
    Ptr<TocinoNetDevice> netDeviceB = CreateNetDeviceHelper( ADDR_B );
    Ptr<TocinoNetDevice> netDeviceC = CreateNetDeviceHelper( ADDR_C );
    Ptr<TocinoNetDevice> netDeviceD = CreateNetDeviceHelper( ADDR_D );

    TocinoChannelHelper( netDeviceA, 0, netDeviceB, 1 );
    TocinoChannelHelper( netDeviceB, 1, netDeviceA, 0 );
    
    TocinoChannelHelper( netDeviceB, 2, netDeviceC, 3 );
    TocinoChannelHelper( netDeviceC, 3, netDeviceB, 2 );
  
    TocinoChannelHelper( netDeviceC, 1, netDeviceD, 0 );
    TocinoChannelHelper( netDeviceD, 0, netDeviceC, 1 );
    
    TocinoChannelHelper( netDeviceD, 3, netDeviceA, 3 );
    TocinoChannelHelper( netDeviceA, 2, netDeviceD, 2 );
    
    // A -> C
    // B -> D
    // C -> A
    // D -> B

    for( unsigned i = 0; i < COUNT; ++i )
    {
        netDeviceA->Send( p, ADDR_C, 0 );
        netDeviceB->Send( p, ADDR_D, 0 );
        netDeviceC->Send( p, ADDR_A, 0 );
        netDeviceD->Send( p, ADDR_B, 0 );
    }

    fprintf( stderr, "Run\n" );

    Simulator::Run();
    Simulator::Destroy();
}

void
TestTocinoDeadlock::DoRun (void)
{
    Config::SetDefault("ns3::TocinoDimensionOrderRouter::WrapPoint", IntegerValue( 2 ) );
    Config::SetDefault("ns3::CallbackQueue::Depth", UintegerValue( 1 ));
    
    TestHelper( 3, 20 );
}
