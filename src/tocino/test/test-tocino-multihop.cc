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
#include "ns3/tocino-misc.h"

#include "test-tocino-multihop.h"

using namespace ns3;

TestTocinoMultihop::TestTocinoMultihop()
  : TestCase( "Test requiring multiple hops" )
{
}

TestTocinoMultihop::~TestTocinoMultihop() {}

namespace
{
    const TocinoAddress ADDR_A(0,0,0);
    const TocinoAddress ADDR_B(1,0,0);
    const TocinoAddress ADDR_C(2,0,0);
    const TocinoAddress ADDR_D(3,0,0);

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

Ptr<TocinoNetDevice> TestTocinoMultihop::CreateNetDeviceHelper( const TocinoAddress& a )
{
    Ptr<TocinoNetDevice> netDevice = CreateObject<TocinoNetDevice>();
    
    netDevice->Initialize();
    netDevice->SetAddress( a );
    netDevice->SetReceiveCallback( MakeCallback( AcceptPacket ) );

    Ptr<Node> node = CreateObject<Node>();
    netDevice->SetNode( node );

    return netDevice;
}

void TestTocinoMultihop::TestHelper(const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
    
    // A <-> B <-> C <-> D
    // (no wrap-around link)
    
    Ptr<TocinoNetDevice> netDeviceA = CreateNetDeviceHelper( ADDR_A );
    Ptr<TocinoNetDevice> netDeviceB = CreateNetDeviceHelper( ADDR_B );
    Ptr<TocinoNetDevice> netDeviceC = CreateNetDeviceHelper( ADDR_C );
    Ptr<TocinoNetDevice> netDeviceD = CreateNetDeviceHelper( ADDR_D );

    // x+ transmission channels
    TocinoChannelHelper( netDeviceA, 0, netDeviceB, 1 );
    TocinoChannelHelper( netDeviceB, 0, netDeviceC, 1 );
    TocinoChannelHelper( netDeviceC, 0, netDeviceD, 1 );

    // x- transmission channels
    TocinoChannelHelper( netDeviceB, 1, netDeviceA, 0 );
    TocinoChannelHelper( netDeviceC, 1, netDeviceB, 0 );
    TocinoChannelHelper( netDeviceD, 1, netDeviceC, 0 );

    Reset();

    Simulator::ScheduleWithContext( netDeviceA->GetNode()->GetId(), Seconds(0), 
        &TocinoNetDevice::Send, netDeviceA, p, ADDR_D, 0 );

    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ( totalCount, 1, "Got unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( totalBytes, BYTES, "Got unexpected total packet bytes" );

#if 0
    bool aq;
    
    aq = netDeviceA->AllQuiet();
    NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 0 not quiet?" );
    aq = netDeviceB->AllQuiet();
    NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 1 not quiet?" );
    aq = netDeviceC->AllQuiet();
    NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 2 not quiet?" );
    aq = netDeviceD->AllQuiet();
    NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 3 not quiet?" );
#endif
}

void
TestTocinoMultihop::DoRun (void)
{
    TocinoCustomizeLogging();
    Config::SetDefault("ns3::CallbackQueue::Depth", UintegerValue(1));

    TestHelper(1000);
    
    Simulator::Destroy();
    Config::Reset();
}
