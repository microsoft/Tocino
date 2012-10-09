/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"

#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/application-container.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-channel.h"

using namespace ns3;

Ptr<TocinoChannel>
LinkHelper(Ptr<TocinoNetDevice> tx_nd,
    uint32_t tx_port,
    Ptr<TocinoNetDevice> rx_nd,
    uint32_t rx_port)
{
    Ptr<TocinoChannel> c = CreateObject<TocinoChannel>();
              
    tx_nd->SetTxChannel(c, tx_port);
    c->SetTransmitter(tx_nd->GetTransmitter(tx_port));

    rx_nd->SetRxChannel(c, rx_port);
    c->SetReceiver(rx_nd->GetReceiver(rx_port));
    return c;
}

int 
main (int argc, char *argv[])
{
    bool verbose = true;
    uint32_t x, y, z;
    uint32_t i;
    uint32_t tx, rx, c;
    uint32_t xp, xm, yp, ym, zp, zm;

    Ptr<TocinoNetDevice> netDevices[3*3*3];
    Ptr<TocinoChannel> channels[3*3*3*6];
    
    CommandLine cmd;
    cmd.AddValue ("verbose", "Tell application to log if true", verbose);
    cmd.Parse (argc,argv);

    Ptr<Node> node;
    NodeContainer nodes;
    nodes.Create(3*3*3);

    // create net devices and connect them to nodes
    NetDeviceContainer ndc = NetDeviceContainer();

    for (x = 0; x < 3; x++)
    { 
        for (y = 0; y < 3; y++)
        { 
            for (z = 0; z < 3; z++)
            {
                i = (x * 9) + (y * 3) + z;
                netDevices[i] = CreateObject<TocinoNetDevice>();
                netDevices[i]->Initialize();
                ndc.Add(netDevices[i]);

                node = nodes.Get(i);
                node->AddDevice(netDevices[i]);
            }
        }
    }

    // create channels, interconnect net devices in 3D torus
    for (x = 0; x < 3; x++)
    { 
        xp = (x + 1) % 3; // x coordinate of neighbor in x+ direction
        xm = (x - 1) % 3; // x coordinate of neighbor in x- direction
        for (y = 0; y < 3; y++)
        { 
            yp = (y + 1) % 3;
            ym = (y - 1) % 3;
            for (z = 0; z < 3; z++)
            {
                zp = (z + 1) % 3;
                zm = (z - 1) % 3;
                
                tx = (x * 9) + (y * 3) + z;
                c = 6 * tx; // base index of a block of 6 channels that tx transmits on
                rx = (xp * 9) + (y * 3) + z;
                channels[c + 0] = LinkHelper(netDevices[tx], 0, netDevices[rx], 1); // x+
                rx = (xm * 9) + (y * 3) + z;
                channels[c + 1] = LinkHelper(netDevices[tx], 1, netDevices[rx], 0); // x-
                rx = (x * 9) + (yp * 3) + z;
                channels[c + 2] = LinkHelper(netDevices[tx], 2, netDevices[rx], 3); // y+
                rx = (x * 9) + (ym * 3) + z;
                channels[c + 3] = LinkHelper(netDevices[tx], 3, netDevices[rx], 2); // y-
                rx = (x * 9) + (y * 3) + zp;
                channels[c + 4] = LinkHelper(netDevices[tx], 4, netDevices[rx], 5); // z+
                rx = (x * 9) + (y * 3) + zm;
                channels[c + 5] = LinkHelper(netDevices[tx], 5, netDevices[rx], 4); // z-
            }
        }
    }

    // install Internet stacks
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = addr.Assign(ndc);

    // configure applications - echo server on node 0, echo clients everywhere else
    // cookbook from Tutorial
    UdpEchoServerHelper echoServer(7); // listening on port 7
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 7);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps;    
    for (i = 1; i < 3*3*3; i++)
    {
        clientApps.Add(echoClient.Install (nodes.Get (i)));
    }
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    // cut it loose
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
