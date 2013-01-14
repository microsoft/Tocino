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

TestTocino3DTorus::TestTocino3DTorus()
  : TestCase( "Test a 3D Torus" )
{}

TestTocino3DTorus::~TestTocino3DTorus()
{}

int TestTocino3DTorus::Inc( const int i ) const
{
    return ( (i + m_radix) + 1 ) % m_radix;
}

int TestTocino3DTorus::Dec( const int i ) const
{
    return ( (i + m_radix) - 1 ) % m_radix;
}

bool TestTocino3DTorus::AcceptPacket( Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t, const Address& src )
{
    TocinoAddress tsrc = TocinoAddress::ConvertFrom( src );
    TocinoAddress tdst = TocinoAddress::ConvertFrom( nd->GetAddress() );

    m_counts[tsrc][tdst]++;
    m_bytes[tsrc][tdst] += p->GetSize();

    return true;
}

void TestTocino3DTorus::Initialize()
{
    NS_ASSERT( m_radix >= 0 );

    // create net devices
    m_netDevices.clear();
    m_netDevices.resize(m_radix);
    for( int x = 0; x < m_radix; x++ )
    { 
        m_netDevices[x].resize(m_radix);
        for( int y = 0; y < m_radix; y++ )
        { 
            m_netDevices[x][y].resize(m_radix);
            for( int z = 0; z < m_radix; z++ )
            {
                Ptr<TocinoNetDevice> tnd = CreateObject<TocinoNetDevice>();
                
                tnd->Initialize();
                tnd->SetAddress( TocinoAddress( x, y, z ) );
                tnd->SetReceiveCallback( MakeCallback( &TestTocino3DTorus::AcceptPacket, this ) );

                // HACK: The Nodes are required to avoid
                // SIGSEGV in TocinoChannel::TransmitEnd()
                tnd->SetNode( CreateObject<Node>() );

                m_netDevices[x][y][z] = tnd;
            }
        }
    }
 
    // create channels and interconnect net devices
    for( int x = 0; x < m_radix; x++ )
    { 
        for( int y = 0; y < m_radix; y++ )
        { 
            for( int z = 0; z < m_radix; z++ )
            {
                Ptr<TocinoNetDevice> cur = m_netDevices[x][y][z];

                TocinoChannelHelper( cur, 0, m_netDevices[ Inc(x) ][y][z], 1 ); // x+
                TocinoChannelHelper( cur, 1, m_netDevices[ Dec(x) ][y][z], 0 ); // x-

                TocinoChannelHelper( cur, 2, m_netDevices[x][ Inc(y) ][z], 3 ); // y+
                TocinoChannelHelper( cur, 3, m_netDevices[x][ Dec(y) ][z], 2 ); // y-

                TocinoChannelHelper( cur, 4, m_netDevices[x][y][ Inc(z) ], 5 ); // z+
                TocinoChannelHelper( cur, 5, m_netDevices[x][y][ Dec(z) ], 4 ); // z-
            }
        }
    }
}

void TestTocino3DTorus::CheckAllQuiet()
{
    bool aq = true;

    for( int x = 0; x < m_radix; x++ )
    { 
        for( int y = 0; y < m_radix; y++ )
        { 
            for( int z = 0; z < m_radix; z++ )
            {
                aq &= m_netDevices[x][y][z]->AllQuiet();
            }
        }
    }
  
    if( aq ) return;

    for( int x = 0; x < m_radix; x++ )
    { 
        for( int y = 0; y < m_radix; y++ )
        { 
            for( int z = 0; z < m_radix; z++ )
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
    const int MAX_COORD = m_radix-1;

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
    for( int x = 0; x < m_radix; x += (m_radix-1) )
    { 
        for( int y = 0; y < m_radix; y += (m_radix-1) )
        { 
            for( int z = 0; z < m_radix; z += (m_radix-1) )
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
    return ( m_radix-1 ) / 2;
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
        for( int x = 0; x < m_radix; x++ )
        { 
            for( int y = 0; y < m_radix; y++ )
            { 
                for( int z = 0; z < m_radix; z++ )
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
    
    for( int x = 0; x < m_radix; x++ )
    { 
        for( int y = 0; y < m_radix; y++ )
        { 
            for( int z = 0; z < m_radix; z++ )
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

    for( int sx = 0; sx < m_radix; sx++ )
    { 
        for( int sy = 0; sy < m_radix; sy++ )
        { 
            for( int sz = 0; sz < m_radix; sz++ )
            {
                TocinoAddress src( sx, sy, sz );

                for( int dx = 0; dx < m_radix; dx++ )
                { 
                    for( int dy = 0; dy < m_radix; dy++ )
                    { 
                        for( int dz = 0; dz < m_radix; dz++ )
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

    const int NODES = m_radix * m_radix * m_radix;
    
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
    m_radix = 3;

    Initialize();
    TestHelper();
   
    Config::SetDefault( "ns3::TocinoDimensionOrderRouter::WrapPoint",
            UintegerValue( m_radix-1 ) );

    Initialize();
    TestHelper();

    Config::Reset();
}
