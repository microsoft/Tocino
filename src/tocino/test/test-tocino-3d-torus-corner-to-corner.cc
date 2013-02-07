/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include "ns3/tocino-net-device.h"

#include "test-tocino-3d-torus-corner-to-corner.h"
#include "tocino-test-results.h"

using namespace ns3;

TestTocino3DTorusCornerToCorner::TestTocino3DTorusCornerToCorner( uint32_t radix, bool doWrap )
    : TestTocino3DTorus( radix, doWrap, " with corner-to-corner traffic" ) 
    , MAX_COORD( radix - 1 )
{}

bool
TestTocino3DTorusCornerToCorner::IsCorner(
        uint32_t idx ) const
{
    const TocinoAddress ta = m_helper.IndexToTocinoAddress( idx );
    
    const uint32_t x = ta.GetX();
    const uint32_t y = ta.GetY();
    const uint32_t z = ta.GetZ();

    if( ( ( x == 0 ) || ( x == MAX_COORD ) ) &&
        ( ( y == 0 ) || ( y == MAX_COORD ) ) &&
        ( ( z == 0 ) || ( z == MAX_COORD ) ) )
    {
        return true;
    }
    
    return false;
}

uint32_t
TestTocino3DTorusCornerToCorner::GetOppositeCorner(
        const uint8_t idx ) const
{
    const TocinoAddress ta = m_helper.IndexToTocinoAddress( idx );
    
    const uint32_t x = ta.GetX();
    const uint32_t y = ta.GetY();
    const uint32_t z = ta.GetZ();

    NS_ASSERT( x == 0 || x == MAX_COORD );
    NS_ASSERT( y == 0 || y == MAX_COORD );
    NS_ASSERT( z == 0 || z == MAX_COORD );

    uint8_t ox = (x == 0) ? MAX_COORD : 0;
    uint8_t oy = (y == 0) ? MAX_COORD : 0;
    uint8_t oz = (z == 0) ? MAX_COORD : 0;

    return m_helper.CoordinatesToIndex( ox, oy, oz );
}

void
TestTocino3DTorusCornerToCorner::TestHelper(
        const Time testDuration,
        const unsigned BYTES )
{
    for( uint32_t src = 0; src < NODES; ++src )
    {
        if( !IsCorner( src ) )
        {
            continue;
        }
            
        NodeContainer machines;
        TocinoTestResults results;

        TocinoCustomizeLogging();

        machines.Create( NODES );

        Tocino3DTorusNetDeviceContainer netDevices =
            m_helper.Install( machines );
  
        m_trafficMatrix.assign( NODES, TocinoTrafficVector( NODES, 0 ) );

        const uint32_t dst = GetOppositeCorner( src );

        m_trafficMatrix[src][dst] = TOCINO_TOTAL_TRAFFIC;
   
        Ptr<TocinoTrafficMatrixApplication> srcApp =
                CreateObject<TocinoTrafficMatrixApplication>();
        
        Ptr<TocinoTrafficMatrixApplication> dstApp =
                CreateObject<TocinoTrafficMatrixApplication>();

        srcApp->Initialize( src, &machines, m_trafficMatrix );
        dstApp->Initialize( dst, &machines, m_trafficMatrix );
        
        srcApp->ResetStatistics();
        dstApp->ResetStatistics();

        srcApp->SetReceiveCallback( 
                MakeCallback( &TocinoTestResults::AcceptPacket, &results ) );
        
        dstApp->SetReceiveCallback( 
                MakeCallback( &TocinoTestResults::AcceptPacket, &results ) );
        
        srcApp->SetStartTime( Seconds( 0.0 ) );
        dstApp->SetStartTime( Seconds( 0.0 ) );
        
        srcApp->SetStopTime( testDuration );
        dstApp->SetStopTime( testDuration );

        srcApp->SetPacketSize( BYTES );
        dstApp->SetPacketSize( BYTES );
        
        machines.Get( src )->AddApplication( srcApp );
        machines.Get( dst )->AddApplication( dstApp );
        
        Simulator::Run();
    
        //NS_LOG_UNCOND( results.ToString() );

        CheckAllQuiet( netDevices );

        const uint32_t COUNT = srcApp->GetPacketsSent();

        const TocinoAddress srcAdd = m_helper.IndexToTocinoAddress( src );
        const TocinoAddress dstAdd = m_helper.IndexToTocinoAddress( dst );

        NS_TEST_ASSERT_MSG_EQ(
                results.GetCount( srcAdd, dstAdd ),
                COUNT,
                "Unexpected packet count" );

        NS_TEST_ASSERT_MSG_EQ(
                results.GetBytes( srcAdd, dstAdd ),
                BYTES*COUNT,
                "Unexpected packet bytes" );

        NS_TEST_ASSERT_MSG_EQ(
                results.GetTotalCount(),
                COUNT,
                "Unexpected total packet count" );

        NS_TEST_ASSERT_MSG_EQ(
                results.GetTotalBytes(),
                BYTES*COUNT,
                "Unexpected total packet bytes" );

        Simulator::Destroy();
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
    
    TestHelper( Seconds( 0.5 ), 20 );
    TestHelper( Seconds( 0.5 ), 123 );
    TestHelper( Seconds( 0.5 ), 32 );

    Config::Reset();
}
