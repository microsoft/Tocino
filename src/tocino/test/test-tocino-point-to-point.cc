/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <vector>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/node.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-flit-header.h"
#include "ns3/tocino-channel.h"
#include "ns3/tocino-helper.h"

#include "test-tocino-point-to-point.h"

using namespace ns3;

TestTocinoPointToPoint::TestTocinoPointToPoint()
  : TestCase( "Send between two directly-connected net devices" )
{
}

TestTocinoPointToPoint::~TestTocinoPointToPoint() {}

namespace
{
    const TocinoAddress ADDR_ONE(0,0,0);
    const TocinoAddress ADDR_TWO(1,0,0);

    unsigned totalCount;
    unsigned totalBytes;
    
    void Reset()
    {
        totalCount = 0;
        totalBytes = 0;
    }

    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet> p, uint16_t, const Address& )
    {
        totalCount++;
        totalBytes += p->GetSize();
        
        return true;
    }
}

Ptr<TocinoNetDevice> TestTocinoPointToPoint::CreateNetDeviceHelper( const TocinoAddress& a )
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

void TestTocinoPointToPoint::TestHelper( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
    
    Ptr<TocinoNetDevice> netDeviceOne = CreateNetDeviceHelper( ADDR_ONE );
    Ptr<TocinoNetDevice> netDeviceTwo = CreateNetDeviceHelper( ADDR_TWO );

    TocinoChannelHelper( netDeviceOne, 0, netDeviceTwo, 1 );
    TocinoChannelHelper( netDeviceTwo, 1, netDeviceOne, 0 );

    Reset();

    for( unsigned i = 0; i < COUNT; ++i )
    {
        Simulator::ScheduleWithContext( netDeviceOne->GetNode()->GetId(), Seconds(0),
            &TocinoNetDevice::Send, netDeviceOne, p, ADDR_TWO, 0 );

        Simulator::ScheduleWithContext( netDeviceTwo->GetNode()->GetId(), Seconds(0),
            &TocinoNetDevice::Send, netDeviceTwo, p, ADDR_ONE, 0 );
    }

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ( totalCount, 2*COUNT, "Got unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( totalBytes, 2*BYTES*COUNT, "Got unexpected total packet bytes" );

    bool aq;
    
    aq = netDeviceOne->AllQuiet();
    NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 1 not quiet?" );
    
    aq = netDeviceTwo->AllQuiet();
    NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 2 not quiet?" );
}

void
TestTocinoPointToPoint::DoRun (void)
{
    Config::SetDefault("ns3::CallbackQueue::Depth", UintegerValue(4));

    TestHelper( 1, 20 );
    TestHelper( 1, 123 );
    TestHelper( 2, 32 );
    TestHelper( 3, 32 );
}
