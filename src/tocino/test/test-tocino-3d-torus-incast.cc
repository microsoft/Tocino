/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include "ns3/tocino-net-device.h"

#include "test-tocino-3d-torus-incast.h"
#include "tocino-test-results.h"

using namespace ns3;

TestTocino3DTorusIncast::TestTocino3DTorusIncast( uint32_t radix, bool doWrap )
    : TestTocino3DTorus( radix, doWrap, " with incast traffic" )
{
    m_trafficMatrix.resize( NODES );
    
    for( uint32_t src = 0; src < NODES; ++src )
    {
        m_trafficMatrix[src].resize( NODES );
        
        for( uint32_t dst = 0; dst < NODES; ++dst )
        {
            // 6:1 incast on the center node
            if( IsCenterNeighbor( src ) && IsCenter( dst ) )
            {
                m_trafficMatrix[src][dst] = TOCINO_TOTAL_TRAFFIC;
            }
            else
            {
                m_trafficMatrix[src][dst] = 0;
            }
        }
    }
}

int
TestTocino3DTorusIncast::Middle() const
{
    return ( RADIX-1 ) / 2;
}

bool
TestTocino3DTorusIncast::IsCenter( const uint32_t idx ) const
{
    const TocinoAddress ta = m_helper.IndexToTocinoAddress( idx );
   
    if( ( ta.GetX() == Middle() ) &&
        ( ta.GetY() == Middle() ) &&
        ( ta.GetZ() == Middle() ) )
    {
        return true;
    }

    return false;
}

bool
TestTocino3DTorusIncast::IsCenterNeighbor( const uint32_t idx ) const
{
    const TocinoAddress ta = m_helper.IndexToTocinoAddress( idx );
    
    int exact = 0;
    int offByOne = 0;

    int dx = abs( Middle() - ta.GetX() );
    int dy = abs( Middle() - ta.GetY() );
    int dz = abs( Middle() - ta.GetZ() );

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
        const Time testDuration,
        const unsigned BYTES )
{
    NodeContainer machines;
    TocinoTestResults results;
    AppVector applications;
    
    TocinoCustomizeLogging();
    
    machines.Create( NODES );
    
    Tocino3DTorusNetDeviceContainer netDevices =
        m_helper.Install( machines );
  
    for( uint32_t node = 0; node < NODES; ++node )
    {
        Ptr<TocinoTrafficMatrixApplication> app =
                CreateObject<TocinoTrafficMatrixApplication>();
    
        applications.push_back(app);

        app->Initialize( node, &machines, m_trafficMatrix );
        app->ResetStatistics();

        app->SetReceiveCallback( 
                MakeCallback( &TocinoTestResults::AcceptPacket, &results ) );
        
        app->SetStartTime( Seconds( 0.0 ) );
        app->SetStopTime( testDuration );
        app->SetPacketSize( BYTES );
        
        machines.Get( node )->AddApplication( app );
    }

    Simulator::Run();
   
    CheckAllQuiet( netDevices );
    
    //NS_LOG_UNCOND( results.ToString() );
  
    const TocinoAddress center( Middle(), Middle(), Middle() );

    for( uint32_t src = 0; src < NODES; ++src )
    {
        for( uint32_t dst = 0; dst < NODES; ++dst )
        {
            if( IsCenterNeighbor( src ) )
            {
                TocinoAddress sa =
                    m_helper.IndexToTocinoAddress( src );

                const uint32_t PACKETS =
                    applications[src]->GetPacketsSent();

                NS_TEST_ASSERT_MSG_EQ(
                        results.GetCount( sa, center ),
                        PACKETS,
                        "Unexpected packet count" );

                NS_TEST_ASSERT_MSG_EQ(
                        results.GetBytes( sa, center ),
                        BYTES * PACKETS,
                        "Unexpected packet bytes" );
            }
        }
    }

    const uint32_t TOTAL_PACKETS = GetTotalPacketsSent( applications );

    NS_TEST_ASSERT_MSG_EQ(
            results.GetTotalCount(),
            TOTAL_PACKETS,
            "Unexpected total packet count" );

    NS_TEST_ASSERT_MSG_EQ(
            results.GetTotalBytes(),
            BYTES * TOTAL_PACKETS,
            "Unexpected total packet bytes" );

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
    
    TestHelper( Seconds( 0.5 ), 20 );
    TestHelper( Seconds( 0.5 ), 123 );
    TestHelper( Seconds( 0.5 ), 32 );
    TestHelper( Seconds( 0.5 ), 458 );

    Config::Reset();
}
