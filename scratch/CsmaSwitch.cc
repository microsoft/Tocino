/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/csma-module.h"
#include "ns3/bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/on-off-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-sink-helper.h"

#include "utils.h"

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CsmaSwitch");

int main (int argc, char *argv[])
{
	const int NUM_MACHINES = 2;
	
	NodeContainer machines;
	machines.Create( NUM_MACHINES );

	NodeContainer tor;
	tor.Create(1);
	
	InternetStackHelper internet;
	internet.Install( machines );

	CsmaHelper csma;
	csma.SetChannelAttribute( "DataRate", DataRateValue( DataRate( "5Mbps" ) ) );
	csma.SetChannelAttribute( "Delay", TimeValue( MilliSeconds( 2 ) ) );

	NetDeviceContainer machineDevs;
	NetDeviceContainer torDevs;

	Ipv4AddressHelper ipv4Helper;
	ipv4Helper.SetBase( "10.1.1.0", Ipv4Mask( "/24" ) );

	for( int i = 0; i < NUM_MACHINES; i++ )
	{
		NetDeviceContainer link = csma.Install( 
			NodeContainer(machines.Get(i), tor) );

		machineDevs.Add( link.Get(0) );
		torDevs.Add( link.Get(1) );	

		ipv4Helper.Assign( link.Get(0) );
	}
	
	BridgeHelper bridge;
	bridge.Install( tor.Get(0), torDevs );
	
	cout << "Machines:" << endl;
	DumpRoutingTables( machines );
	
	const int PORT = 9;

	// odd machines send, even machines receive
	for( int i = 0; i < NUM_MACHINES; i += 2 )
	{
		ApplicationContainer ac;
		
		PacketSinkHelper sink( "ns3::UdpSocketFactory", InetSocketAddress( Ipv4Address::GetAny(), PORT ) );

		ac = sink.Install( machines.Get( i+1 ) );
		ac.Start( Seconds( 0.0 ) );

		Ipv4Address sinkAddress =
			machines.Get( i+1 )->GetObject<Ipv4>()->GetAddress( 1, 0 ).GetLocal();

		OnOffHelper source( "ns3::UdpSocketFactory", InetSocketAddress( sinkAddress, PORT ) );
		
		source.SetConstantRate( DataRate( "500kb/s" ) );

		ac = source.Install( machines.Get( i ) );
		ac.Start( Seconds( 1.0 ) );
		ac.Stop( Seconds( 6.0 ) );
	}

	// write a tcp dump trace
	csma.EnablePcapAll( "CsmaSwitch", false );
	
	NS_LOG_UNCOND( "Run Simulation" );
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_UNCOND( "Done" );
}
