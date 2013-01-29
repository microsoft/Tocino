/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-channel.h"
#include "ns3/tocino-helper.h"
#include "ns3/tocino-misc.h"

#include "test-tocino-3d-torus.h"

using namespace ns3;

TestTocino3DTorus::TestTocino3DTorus( uint32_t radix, bool doWrap )
    : TestCase( "Test a 3D Torus" )
    , RADIX( radix )
    , NODES( radix * radix * radix )
    , m_doWrap( doWrap )
{}

TestTocino3DTorus::~TestTocino3DTorus()
{}

bool TestTocino3DTorus::AcceptPacket( Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t, const Address& src )
{
    TocinoAddress tsrc = TocinoAddress::ConvertFrom( src );
    TocinoAddress tdst = TocinoAddress::ConvertFrom( nd->GetAddress() );

    m_counts[tsrc][tdst]++;
    m_bytes[tsrc][tdst] += p->GetSize();

    return true;
}

void TestTocino3DTorus::CheckAllQuiet()
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

void TestTocino3DTorus::Reset()
{
    m_counts.clear();
    m_bytes.clear();
}

unsigned TestTocino3DTorus::GetTotalCount() const
{
    unsigned total = 0;

    TestMatrix::const_iterator i;
    TestMatrixRow::const_iterator j;

    for( i = m_counts.begin(); i != m_counts.end(); i++ ) 
    {
        const TestMatrixRow& row = m_counts.find(i->first)->second;

        for( j = row.begin(); j != row.end(); j++ ) 
        {
            total += j->second;
        }
    }

    return total;
}

unsigned TestTocino3DTorus::GetTotalBytes() const
{
    unsigned total = 0;
    
    TestMatrix::const_iterator i;
    TestMatrixRow::const_iterator j;
    
    for( i = m_bytes.begin(); i != m_bytes.end(); i++ ) 
    {
        const TestMatrixRow& row = m_bytes.find(i->first)->second;

        for( j = row.begin(); j != row.end(); j++ ) 
        {
            total += j->second;
        }
    }

    return total;
}

TocinoAddress TestTocino3DTorus::OppositeCorner( const uint8_t x, const uint8_t y, const uint8_t z )
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

void TestTocino3DTorus::TestCornerToCorner( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );

    // iterate over the "corners"
    for( uint32_t x = 0; x < RADIX; x += (RADIX-1) )
    { 
        for( uint32_t y = 0; y < RADIX; y += (RADIX-1) )
        { 
            for( uint32_t z = 0; z < RADIX; z += (RADIX-1) )
            {
                Reset();
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

                NS_TEST_ASSERT_MSG_EQ( m_counts[src][dst], COUNT, "Unexpected packet count" );
                NS_TEST_ASSERT_MSG_EQ( m_bytes[src][dst], BYTES*COUNT, "Unexpected packet bytes" );

                NS_TEST_ASSERT_MSG_EQ( GetTotalCount(), COUNT, "Unexpected total packet count" );
                NS_TEST_ASSERT_MSG_EQ( GetTotalBytes(), BYTES*COUNT, "Unexpected total packet bytes" );

                Simulator::Destroy();
            }
        }
    }
}

int TestTocino3DTorus::Middle() const
{
    return ( RADIX-1 ) / 2;
}

bool TestTocino3DTorus::IsCenterNeighbor( const int x, const int y, const int z ) const
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

void TestTocino3DTorus::TestIncast( const unsigned COUNT, const unsigned BYTES )
{
    // 6:1 incast on the center node
    
    Reset();
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

                    NS_TEST_ASSERT_MSG_EQ( m_counts[src][center], COUNT, "Unexpected packet count" );
                    NS_TEST_ASSERT_MSG_EQ( m_bytes[src][center], BYTES*COUNT, "Unexpected packet bytes" );
                }
            }
        }
    }

    NS_TEST_ASSERT_MSG_EQ( GetTotalCount(), 6*COUNT, "Unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( GetTotalBytes(), 6*BYTES*COUNT, "Unexpected total packet bytes" );
    
    Simulator::Destroy();
}

void TestTocino3DTorus::TestAllToAll( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
                
    Reset();
    TocinoCustomizeLogging();

    for( uint32_t sx = 0; sx < RADIX; sx++ )
    { 
        for( uint32_t sy = 0; sy < RADIX; sy++ )
        { 
            for( uint32_t sz = 0; sz < RADIX; sz++ )
            {
                TocinoAddress src( sx, sy, sz );

                for( uint32_t dx = 0; dx < RADIX; dx++ )
                { 
                    for( uint32_t dy = 0; dy < RADIX; dy++ )
                    { 
                        for( uint32_t dz = 0; dz < RADIX; dz++ )
                        {
                            TocinoAddress dst( dx, dy, dz );

                            if( src == dst )
                            {
                                // don't send to self
                                continue;
                            }

                            Ptr<TocinoNetDevice> srcNetDevice = m_netDevices[sx][sy][sz];

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
                        }
                    }
                }
            }
        }
    }
    
    Simulator::Run();

    CheckAllQuiet();

    NS_TEST_ASSERT_MSG_EQ( GetTotalCount(), NODES*(NODES-1)*COUNT,
            "Unexpected total packet count" );

    NS_TEST_ASSERT_MSG_EQ( GetTotalBytes(), NODES*(NODES-1)*BYTES*COUNT,
            "Unexpected total packet bytes" );

    Simulator::Destroy();
}
void TestTocino3DTorus::TestHelper()
{
    TestCornerToCorner( 1, 20 );
    TestCornerToCorner( 1, 123 );
    TestCornerToCorner( 10, 32 );
    
    TestIncast( 1, 20 );
    TestIncast( 1, 123 );
    TestIncast( 10, 32 );
    TestIncast( 5, 458 );
    
    TestAllToAll( 1, 20 );
    TestAllToAll( 1, 123 );
    TestAllToAll( 10, 32 );
}

void
TestTocino3DTorus::DoRun()
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
        nd->SetReceiveCallback( MakeCallback( &TestTocino3DTorus::AcceptPacket, this ) );
    }

    TestHelper();

    Config::Reset();
}
