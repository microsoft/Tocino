#ifndef __UTILS_H_
#define __UTILS_H_

#include <iostream>

#include "ns3/node-container.h"
#include "ns3/internet-module.h"

void DumpRoutingTables( ns3::NodeContainer& nc )
{
	using namespace std;
	using namespace ns3;

	const int NODES = nc.GetN();

	for( int n = 0; n < NODES; n++ )
	{
		cout << "------------" << endl;
		
		Ptr<Ipv4> ipv4 = nc.Get(n)->GetObject<Ipv4>();
	
		if( ipv4 == NULL ) continue;

		const int INTERFACES = ipv4->GetNInterfaces();

		for( int i = 0; i < INTERFACES; i++ )
		{
			const int ADDRESSES = ipv4->GetNAddresses( i );

			for( int a = 0; a < ADDRESSES; a++ )
			{
				ipv4->GetAddress(i,a).GetLocal().Print(cout);
				cout << endl;
			}
		}
		
		cout << endl;

		OutputStreamWrapper osw = OutputStreamWrapper( &cout );

		cout << "Static:" << endl;
		Ipv4StaticRoutingHelper helper;
		Ptr<Ipv4StaticRouting> ipv4stat = helper.GetStaticRouting( ipv4 );
		ipv4stat->PrintRoutingTable( Ptr<OutputStreamWrapper>(&osw) );
		cout << endl;

		Ptr<GlobalRouter> gr = nc.Get(n)->GetObject<GlobalRouter>();
		
		cout << "Global:" << endl;
		Ptr<Ipv4GlobalRouting> ipv4gr = gr->GetRoutingProtocol();
		ipv4gr->PrintRoutingTable( Ptr<OutputStreamWrapper>(&osw) );
		cout << endl;
	}
}
#endif // __UTILS_H
