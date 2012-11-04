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

#include "test-tocino-multihop.h"

using namespace ns3;

TestTocinoMultihop::TestTocinoMultihop()
  : TestCase( "Send multihop packet net devices" )
{
}

TestTocinoMultihop::~TestTocinoMultihop() {}

namespace
{
    const TocinoAddress ADDR_ZERO(0,0,0);
    const TocinoAddress ADDR_ONE(1,0,0);
    const TocinoAddress ADDR_TWO(2,0,0);
    const TocinoAddress ADDR_THREE(3,0,0);

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
    
    Ptr<TocinoNetDevice> netDeviceZero = CreateNetDeviceHelper( ADDR_ZERO );
    Ptr<TocinoNetDevice> netDeviceOne = CreateNetDeviceHelper( ADDR_ONE );
    Ptr<TocinoNetDevice> netDeviceTwo = CreateNetDeviceHelper( ADDR_TWO );
    Ptr<TocinoNetDevice> netDeviceThree = CreateNetDeviceHelper( ADDR_THREE );

    // 4 element ring
    // x+ transmission channels
    TocinoChannelHelper( netDeviceZero, 0, netDeviceOne, 1 );
    TocinoChannelHelper( netDeviceOne, 0, netDeviceTwo, 1 );
    TocinoChannelHelper( netDeviceTwo, 0, netDeviceThree, 1 );
    TocinoChannelHelper( netDeviceThree, 0, netDeviceZero, 1 );

    // x- transmission channels
    TocinoChannelHelper( netDeviceZero, 1, netDeviceThree, 0 );
    TocinoChannelHelper( netDeviceOne, 1, netDeviceZero, 0 );
    TocinoChannelHelper( netDeviceTwo, 1, netDeviceOne, 0 );
    TocinoChannelHelper( netDeviceThree, 1, netDeviceTwo, 0 );

    Reset();

    netDeviceZero->Send( p, ADDR_TWO, 0 );

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ( totalCount, 1, "Got unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( totalBytes, BYTES, "Got unexpected total packet bytes" );

    //bool aq;
    
    // aq = netDeviceZero->AllQuiet();
    // NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 0 not quiet?" );
    // aq = netDeviceOne->AllQuiet();
    // NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 1 not quiet?" );
    // aq = netDeviceTwo->AllQuiet();
    // NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 2 not quiet?" );
    // aq = netDeviceThree->AllQuiet();
    // NS_TEST_ASSERT_MSG_EQ( aq, true, "Net device 3 not quiet?" );
}

void
TestTocinoMultihop::DoRun (void)
{
    Config::SetDefault("ns3::CallbackQueue::Depth", UintegerValue(1));

    TestHelper(1000);
}
