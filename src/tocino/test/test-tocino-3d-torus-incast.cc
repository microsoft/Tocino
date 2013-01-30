/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

#include "ns3/tocino-net-device.h"

#include "test-tocino-3d-torus-incast.h"

using namespace ns3;

TestTocino3DTorusIncast::TestTocino3DTorusIncast( uint32_t radix, bool doWrap )
    : TestCase( doWrap ?
            "Test a 3D Torus with Incast Traffic" :
            "Test a 3D Mesh with Incast Traffic" )
    , RADIX( radix )
    , NODES( radix * radix * radix )
    , m_doWrap( doWrap )
{}

void
TestTocino3DTorusIncast::CheckAllQuiet()
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

int
TestTocino3DTorusIncast::Middle() const
{
    return ( RADIX-1 ) / 2;
}

bool
TestTocino3DTorusIncast::IsCenterNeighbor( const int x, const int y, const int z ) const
{
    int exact = 0;
    int offByOne = 0;

    int dx = abs( Middle() - x );
    int dy = abs( Middle() - y );
    int dz = abs( Middle() - z );

    if( dx == 0 ) { exact++; }
    else if( dx == 1 ) { offByOne++; }

    if( dy == 0 ) { exact++; }
    else if( dy == 1 ) { offByOne++; }
    
    if( dz == 0 ) { exact++; }
    else if( dz == 1 ) { offByOne++; }

    if( exact == 2 && offByOne == 1 )
    {
        return true;
    }

    return false;
}

void
TestTocino3DTorusIncast::TestHelper(
        const unsigned COUNT,
        const unsigned BYTES )
{
    // 6:1 incast on the center node
    
    m_results.Reset();
    TocinoCustomizeLogging();

    Ptr<Packet> p = Create<Packet>( BYTES );
   
    const TocinoAddress center( Middle(), Middle(), Middle() );
    
    for( unsigned i = 0; i < COUNT; ++i )
    {
        for( uint32_t x = 0; x < RADIX; x++ )
        { 
            for( uint32_t y = 0; y < RADIX; y++ )
            { 
                for( uint32_t z = 0; z < RADIX; z++ )
                {
                    if( IsCenterNeighbor( x, y, z ) )
                    {
                        Simulator::ScheduleWithContext(
                                m_netDevices[x][y][z]->GetNode()->GetId(),
                                Seconds(0),
                                &TocinoNetDevice::Send,
                                m_netDevices[x][y][z],
                                p,
                                center,
                                0 );
                    }
                }
            }
        }
    }

    Simulator::Run();
   
    CheckAllQuiet();
    
    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                if( IsCenterNeighbor( x, y, z ) )
                {
                    TocinoAddress src( x, y, z );

                    NS_TEST_ASSERT_MSG_EQ(
                            m_results.GetCount( src, center ),
                            COUNT,
                            "Unexpected packet count" );

                    NS_TEST_ASSERT_MSG_EQ(
                            m_results.GetBytes( src, center ),
                            BYTES*COUNT,
                            "Unexpected packet bytes" );
                }
            }
        }
    }

    NS_TEST_ASSERT_MSG_EQ(
            m_results.GetTotalCount(), 6*COUNT, "Unexpected total packet count" );

    NS_TEST_ASSERT_MSG_EQ(
            m_results.GetTotalBytes(), 6*BYTES*COUNT, "Unexpected total packet bytes" );
    
    Simulator::Destroy();
}

void
TestTocino3DTorusIncast::DoRun()
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
                MakeCallback( &TocinoTestResults::AcceptPacket, &m_results ) );
    }

    TestHelper( 1, 20 );
    TestHelper( 1, 123 );
    TestHelper( 10, 32 );
    TestHelper( 5, 458 );

    Config::Reset();
}
