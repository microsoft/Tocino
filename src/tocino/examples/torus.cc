/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"

#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"

#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"

#include "ns3/all2all.h"

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

    c->SetReceiver(rx_nd->GetReceiver(rx_port));
    return c;
}

int 
main (int argc, char *argv[])
{
    uint8_t MAXX = 3;
    uint8_t MAXY = 3;
    uint8_t MAXZ = 3;

    bool verbose = true;

    uint32_t x, y, z;
    uint32_t i;
    uint32_t tx, rx, c;
    uint32_t xp, xm, yp, ym, zp, zm;

    Ptr<Node> nodes[MAXX*MAXY*MAXZ];

    Ptr<TocinoNetDevice> netDevices[MAXX*MAXY*MAXZ];
    Ptr<All2All> apps[MAXX*MAXY*MAXZ];
    
    Ptr<TocinoChannel> channels[MAXX*MAXY*MAXZ*6];

    CommandLine cmd;
    cmd.AddValue ("verbose", "Enable application logging", verbose);
    //cmd.AddValue ("X", "X dimension of torus", X);
    //cmd.AddValue ("Y", "Y dimension of torus", Y);
    //cmd.AddValue ("Z", "Z dimension of torus", Z);
    cmd.Parse (argc,argv);

    // create devices, apps, and install them in nodes
    for (x = 0; x < MAXX; x++)
    { 
        for (y = 0; y < MAXY; y++)
        { 
            for (z = 0; z < MAXZ; z++)
            {
                TocinoAddress ta(x, y, z);
                i = (x * MAXY * MAXZ) + (y * MAXZ) + z; // linearized index

                netDevices[i] = CreateObject<TocinoNetDevice>();
                netDevices[i]->Initialize();
                netDevices[i]->SetAddress( ta  ); // implicit conversion

                apps[i] = CreateObject<All2All>();
                apps[i]->SetStartTime(Seconds(1.0));
                apps[i]->SetStopTime(Seconds(1.5));

                nodes[i] = CreateObject<Node>();
                nodes[i]->AddDevice(netDevices[i]);
                nodes[i]->AddApplication(apps[i]);
            }
        }
    }

    // create channels, interconnect net devices in 3D torus
    for (x = 0; x < MAXX; x++)
    { 
        xp = (x + 1) % MAXX; // x coordinate of neighbor in x+ direction
        xm = (x - 1) % MAXX; // x coordinate of neighbor in x- direction
        for (y = 0; y < MAXY; y++)
        { 
            yp = (y + 1) % MAXY;
            ym = (y - 1) % MAXY;
            for (z = 0; z < MAXZ; z++)
            {
                zp = (z + 1) % MAXZ;
                zm = (z - 1) % MAXZ;
                
                tx = (x * MAXY * MAXZ) + (y * MAXZ) + z; // linearize torus space

                c = 6 * tx; // base index of a block of 6 channels that tx transmits on
                rx = (xp * MAXY * MAXZ) + (y * MAXZ) + z;
                channels[c + 0] = LinkHelper(netDevices[tx], 0, netDevices[rx], 1); // x+
                rx = (xm * MAXY * MAXZ) + (y * MAXZ) + z;
                channels[c + 1] = LinkHelper(netDevices[tx], 1, netDevices[rx], 0); // x-
                rx = (x * MAXY * MAXZ) + (yp * MAXZ) + z;
                channels[c + 2] = LinkHelper(netDevices[tx], 2, netDevices[rx], 3); // y+
                rx = (x * MAXY * MAXZ) + (ym * MAXZ) + z;
                channels[c + 3] = LinkHelper(netDevices[tx], 3, netDevices[rx], 2); // y-
                rx = (x * MAXY * MAXZ) + (y * MAXZ) + zp;
                channels[c + 4] = LinkHelper(netDevices[tx], 4, netDevices[rx], 5); // z+
                rx = (x * MAXY * MAXZ) + (y * MAXZ) + zm;
                channels[c + 5] = LinkHelper(netDevices[tx], 5, netDevices[rx], 4); // z-

            }
        }
    }

    // configure apps - talk to all other apps but not self
    for (i = 0; i < (MAXX * MAXY * MAXZ); i++)
    {
        for (uint32_t j = 0; j < (MAXX * MAXY * MAXZ); j++)
        {
            if (i == j) continue; // don't talk to yourself (?)

            apps[i]->AddRemote(netDevices[j]);
        }
    }

    // cut it loose
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
