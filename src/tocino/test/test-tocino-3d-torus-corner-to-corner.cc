/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

#include "ns3/tocino-net-device.h"

#include "test-tocino-3d-torus-corner-to-corner.h"

using namespace ns3;

TestTocino3DTorusCornerToCorner::TestTocino3DTorusCornerToCorner( uint32_t radix, bool doWrap )
    : TestCase( doWrap ?
            "Test a 3D Torus with Corner-to-Corner Traffic" : 
            "Test a 3D Mesh with Corner-to-Corner Traffic" )
    , RADIX( radix )
    , NODES( radix * radix * radix )
    , m_doWrap( doWrap )
{}

void
TestTocino3DTorusCornerToCorner::CheckAllQuiet()
{
    bool aq = true;

    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                aq &= m_netDevices[x][y][z]->AllQuiet();
            }
        }
    }
  
    if( aq ) return;

    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                m_netDevices[x][y][z]->DumpState();
            }
        }
    }

    NS_TEST_ASSERT_MSG_EQ( aq, true, "not all quiet?" );
}

TocinoAddress
TestTocino3DTorusCornerToCorner::OppositeCorner(
        const uint8_t x,
        const uint8_t y,
        const uint8_t z ) const
{
    const int MAX_COORD = RADIX-1;

    NS_ASSERT( x == 0 || x == MAX_COORD );
    NS_ASSERT( y == 0 || y == MAX_COORD );
    NS_ASSERT( z == 0 || z == MAX_COORD );

    uint8_t ox = (x == 0) ? MAX_COORD : 0;
    uint8_t oy = (y == 0) ? MAX_COORD : 0;
    uint8_t oz = (z == 0) ? MAX_COORD : 0;

    return TocinoAddress( ox, oy, oz );
}

void TestTocino3DTorusCornerToCorner::TestHelper( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );

    // iterate over the "corners"
    for( uint32_t x = 0; x < RADIX; x += (RADIX-1) )
    { 
        for( uint32_t y = 0; y < RADIX; y += (RADIX-1) )
        { 
            for( uint32_t z = 0; z < RADIX; z += (RADIX-1) )
            {
                m_results.Reset();
                TocinoCustomizeLogging();

                TocinoAddress src( x, y, z );
                TocinoAddress dst = OppositeCorner( x, y, z );

                Ptr<TocinoNetDevice> srcNetDevice = m_netDevices[x][y][z];

                for( unsigned i = 0; i < COUNT; ++i )
                {
                    Simulator::ScheduleWithContext( 
                            srcNetDevice->GetNode()->GetId(),
                            Seconds(0),
                            &TocinoNetDevice::Send,
                            srcNetDevice,
                            p,
                            dst,
                            0 );
                }

                Simulator::Run();

                CheckAllQuiet();

                NS_TEST_ASSERT_MSG_EQ(
                        m_results.GetCount( src, dst ),
                        COUNT,
                        "Unexpected packet count" );

                NS_TEST_ASSERT_MSG_EQ(
                        m_results.GetBytes( src, dst ),
                        BYTES*COUNT,
                        "Unexpected packet bytes" );

                NS_TEST_ASSERT_MSG_EQ(
                        m_results.GetTotalCount(),
                        COUNT,
                        "Unexpected total packet count" );

                NS_TEST_ASSERT_MSG_EQ(
                        m_results.GetTotalBytes(),
                        BYTES*COUNT,
                        "Unexpected total packet bytes" );

                Simulator::Destroy();
            }
        }
    }
}

void
TestTocino3DTorusCornerToCorner::DoRun()
{
    if( m_doWrap )
    {
        Config::SetDefault( "ns3::TocinoDimensionOrderRouter::WrapPoint",
                UintegerValue( RADIX-1 ) );
    }
    
    Tocino3DTorusTopologyHelper helper( RADIX );
    
    m_machines = NodeContainer();
    m_machines.Create( NODES );
    
    m_netDevices = helper.Install( m_machines );
    
    for( uint32_t n = 0; n < NODES; ++n )
    {
        Ptr<NetDevice> nd = m_machines.Get( n )->GetDevice( 0 );
        
        nd->SetReceiveCallback( 
                MakeCallback( &ns3::TocinoTestResults::AcceptPacket, &m_results ) );
    }
    
    TestHelper( 1, 20 );
    TestHelper( 1, 123 );
    TestHelper( 10, 32 );

    Config::Reset();
}
