/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "test-tocino-ring.h"

#include "ns3/packet.h"
#include "ns3/node.h"

#include "ns3/tocino-helper.h"
#include "ns3/tocino-channel.h"
#include "ns3/simulator.h"

using namespace ns3;

TestTocinoRing::TestTocinoRing()
  : TestCase( "Test three devices in a ring" )
{
}

TestTocinoRing::~TestTocinoRing() {}

namespace
{
    const TocinoAddress ADDR_A(0,0,0);
    const TocinoAddress ADDR_B(1,0,0);
    const TocinoAddress ADDR_C(2,0,0);
   
    const unsigned NODES = 3;
    
    const unsigned SOURCES = NODES;
    const unsigned DESTS = NODES;
    
    const unsigned IDX_A = 0;
    const unsigned IDX_B = 1;
    const unsigned IDX_C = 2;

    unsigned countArray[SOURCES][DESTS];
    unsigned bytesArray[SOURCES][DESTS];
    
    void Reset()
    {
        for( unsigned s = 0; s < SOURCES; ++s )
        {
            for( unsigned d = 0; d < DESTS; ++d )
            {
                countArray[s][d] = 0;
                bytesArray[s][d] = 0;
            }
        }
    }
    
    unsigned GetTotalCount()
    {
        unsigned totalCount = 0;

        for( unsigned s = 0; s < SOURCES; ++s )
        {
            for( unsigned d = 0; d < DESTS; ++d )
            {
                totalCount += countArray[s][d];
            }
        }
        return totalCount;
    }
    
    unsigned GetTotalBytes()
    {
        unsigned totalBytes = 0;

        for( unsigned s = 0; s < SOURCES; ++s )
        {
            for( unsigned d = 0; d < DESTS; ++d )
            {
                totalBytes += bytesArray[s][d];
            }
        }
        return totalBytes;
    }

    unsigned IndexOf( const Address& src )
    {
        TocinoAddress tsrc = TocinoAddress::ConvertFrom( src );
        
        if( tsrc == ADDR_A )
        {
            return IDX_A;
        }
        
        if( tsrc == ADDR_B )
        {
            return IDX_B;
        }
        
        NS_ASSERT( tsrc == ADDR_C );
        return IDX_C;
    }

    // The (non-promiscuous) receive callback provides only the packet's source,
    // not it's destination.  We use a template function here such that each
    // NetDevice has a different callback, specialized on destination.
    template <unsigned DST_IDX>
    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet> p, uint16_t, const Address& src )
    {
        const unsigned SRC_IDX = IndexOf( src );

        countArray[SRC_IDX][DST_IDX]++;
        bytesArray[SRC_IDX][DST_IDX] += p->GetSize();
        return true;
    }
}

template <unsigned IDX>
Ptr<TocinoNetDevice> TestTocinoRing::CreateNetDeviceHelper( const TocinoAddress& a )
{
    Ptr<TocinoNetDevice> netDevice = CreateObject<TocinoNetDevice>();
    
    netDevice->Initialize();
    netDevice->SetAddress( a );
    netDevice->SetReceiveCallback( MakeCallback( AcceptPacket<IDX> ) );

    // HACK: The Nodes are required to avoid
    // SIGSEGV in TocinoChannel::TransmitEnd()
    Ptr<Node> node = CreateObject<Node>();
    netDevice->SetNode( node );
    
    return netDevice;
}

void TestTocinoRing::TestHelper( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
    
    Ptr<TocinoNetDevice> netDeviceA = CreateNetDeviceHelper<IDX_A>( ADDR_A );
    Ptr<TocinoNetDevice> netDeviceB = CreateNetDeviceHelper<IDX_B>( ADDR_B );
    Ptr<TocinoNetDevice> netDeviceC = CreateNetDeviceHelper<IDX_C>( ADDR_C );

    TocinoChannelHelper( netDeviceA, 0, netDeviceB, 1 );
    TocinoChannelHelper( netDeviceB, 1, netDeviceA, 0 );
    
    TocinoChannelHelper( netDeviceB, 0, netDeviceC, 1 );
    TocinoChannelHelper( netDeviceC, 1, netDeviceB, 0 );
   
    TocinoChannelHelper( netDeviceC, 0, netDeviceA, 1 );
    TocinoChannelHelper( netDeviceA, 1, netDeviceC, 0 );

    //
    // A -> C

    Reset();

    for( unsigned i = 0; i < COUNT; ++i )
    {
        netDeviceA->Send( p, ADDR_C, 0 );
    }

    Simulator::Run();
   
    NS_TEST_ASSERT_MSG_EQ( countArray[IDX_A][IDX_C], COUNT, "A->C had unexpected packet count" );
    NS_TEST_ASSERT_MSG_EQ( bytesArray[IDX_A][IDX_C], BYTES*COUNT, "A->C had unexpected packet bytes" );
    
    NS_TEST_ASSERT_MSG_EQ( GetTotalCount(), COUNT, "Unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( GetTotalBytes(), BYTES*COUNT, "Unexpected total packet bytes" );

    // 
    // C -> A 

    Reset();

    for( unsigned i = 0; i < COUNT; ++i )
    {
        netDeviceC->Send( p, ADDR_A, 0 );
    }

    Simulator::Run();
    
    NS_TEST_ASSERT_MSG_EQ( countArray[IDX_C][IDX_A], COUNT, "C->A had unexpected packet count" );
    NS_TEST_ASSERT_MSG_EQ( bytesArray[IDX_C][IDX_A], BYTES*COUNT, "C->A had unexpected packet bytes" );
    
    NS_TEST_ASSERT_MSG_EQ( GetTotalCount(), COUNT, "Unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( GetTotalBytes(), BYTES*COUNT, "Unexpected total packet bytes" );

    Simulator::Destroy();
}

void
TestTocinoRing::DoRun (void)
{
    TestHelper( 1, 20 );
    TestHelper( 1, 123 );
    TestHelper( 2, 32 );
}
