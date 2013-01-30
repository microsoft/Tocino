/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

#include "ns3/tocino-net-device.h"

#include "test-tocino-3d-torus-all-to-all.h"

using namespace ns3;

TestTocino3DTorusAllToAll::TestTocino3DTorusAllToAll( uint32_t radix, bool doWrap )
    : TestCase( doWrap ?
            "Test a 3D Torus with All-to-All Traffic" :
            "Test a 3D Mesh with All-to-All Traffic" )
    , RADIX( radix )
    , NODES( radix * radix * radix )
    , m_doWrap( doWrap )
{}

void
TestTocino3DTorusAllToAll::CheckAllQuiet()
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

void
TestTocino3DTorusAllToAll::TestHelper(
        const unsigned COUNT,
        const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
                
    m_results.Reset();
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

    NS_TEST_ASSERT_MSG_EQ(
            m_results.GetTotalCount(),
            NODES*(NODES-1)*COUNT,
            "Unexpected total packet count" );

    NS_TEST_ASSERT_MSG_EQ(
            m_results.GetTotalBytes(),
            NODES*(NODES-1)*BYTES*COUNT,
            "Unexpected total packet bytes" );

    Simulator::Destroy();
}

void
TestTocino3DTorusAllToAll::DoRun()
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
