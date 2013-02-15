#include <vector>
#include <map>
#include <iostream>

#include "ns3/node-container.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/command-line.h"

#include "ns3/tocino-traffic-matrix-application.h"
#include "ns3/tocino-misc.h"
#include "ns3/tocino-3d-torus-topology-helper.h"
#include "ns3/tocino-net-device.h"

using namespace std;
using namespace ns3;

int main( int argc, char** argv )
{
    unsigned radix = 4;
    unsigned packetBytes = 20;
    Time duration = Seconds( 0.5 );

    CommandLine cmd;
    cmd.AddValue( "radix", "Radix of torus", radix );
    cmd.AddValue( "packetbytes", "Bytes per packet", packetBytes );
    cmd.AddValue( "duration", "Time duration of simulation", duration );
    cmd.Parse( argc, argv );
    
    Config::SetDefault( "ns3::TocinoDimensionOrderRouter::WrapPoint",
            UintegerValue( radix-1 ) );
    
    const unsigned NODES = radix * radix * radix;

    cout << "Simulating "
        << radix << "x"
        << radix << "x" 
        << radix << " torus ("
        << NODES << " total nodes)"
        << endl;
    
    cout << "Sending "
        << packetBytes << " byte packets for "
        << duration.GetMicroSeconds() << " us" << endl;

    NodeContainer machines;
    
    std::vector< Ptr<TocinoTrafficMatrixApplication > > applications;
    
    TocinoCustomizeLogging();
        
    machines.Create( NODES );
   
    Tocino3DTorusTopologyHelper helper( radix );
   
    // all-to-all pattern    
    TocinoTrafficMatrix trafficMatrix( NODES,
            TocinoTrafficVector( NODES, TOCINO_TOTAL_TRAFFIC/NODES ) );
    
    Tocino3DTorusNetDeviceContainer netDevices =
        helper.Install( machines );
  
    for( uint32_t node = 0; node < NODES; ++node )
    {
        Ptr<TocinoTrafficMatrixApplication> app =
                CreateObject<TocinoTrafficMatrixApplication>();
    
        applications.push_back(app);

        app->Initialize( node, &machines, trafficMatrix );
        app->ResetStatistics();

        app->SetStartTime( Seconds( 0.0 ) );
        app->SetStopTime( duration );
        app->SetPacketSize( packetBytes );
        
        machines.Get( node )->AddApplication( app );
    }

    Simulator::Run();

    helper.ReportBisectionBandwidth( netDevices, duration );

    Simulator::Destroy();
}
