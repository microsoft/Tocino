/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-test-results.h"

#include "test-tocino-3d-torus-all-to-all.h"

using namespace ns3;

TestTocino3DTorusAllToAll::TestTocino3DTorusAllToAll(
        uint32_t radix,
        bool doWrap,
        bool doVLB )
    : TestTocino3DTorus( radix, doWrap, doVLB, " with all-to-all traffic" )
{
    m_trafficMatrix.resize( NODES );
    
    for( uint32_t src = 0; src < NODES; ++src )
    {
        m_trafficMatrix[src].resize( NODES );
        
        for( uint32_t dst = 0; dst < NODES; ++dst )
        {
            m_trafficMatrix[src][dst] =
                TOCINO_TOTAL_TRAFFIC/NODES;
        }
    }
}

void
TestTocino3DTorusAllToAll::TestHelper(
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

    //NS_LOG_UNCOND( results.ToString() );

    CheckAllQuiet( netDevices );
   
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
TestTocino3DTorusAllToAll::DoRun()
{
    if( m_doWrap )
    {
        Config::SetDefault( 
                "ns3::TocinoDimensionOrderRouter::EnableWrapAround",
                UintegerValue( RADIX ) );
    }
    
    if( m_doVLB )
    {
        Config::SetDefault( 
                "ns3::TocinoTrafficMatrixApplication::EnableValiantLoadBalancing",
                BooleanValue( true ) );
    }
    
    TestHelper( Seconds( 0.5 ), 20 );
    TestHelper( Seconds( 0.5 ), 123 );
    TestHelper( Seconds( 0.5 ), 32 );

    Config::Reset();
}
