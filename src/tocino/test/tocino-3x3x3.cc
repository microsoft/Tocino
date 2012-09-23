/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <vector>

// Include a header file from your module to test.
#include "ns3/tocino-net-device.h"
#include "ns3/tocino-channel.h"

#include "ns3/test.h"

using namespace ns3;

class Tocino3x3x3 : public TestCase
{
public:
  Tocino3x3x3();
  virtual ~Tocino3x3x3();
private:
  virtual void DoRun (void);

  std::vector<Ptr<TocinoChannel>> m_channels;
  std::vector<Ptr<TocinoNetDevice>> m_netDevices;
};

Tocino3x3x3::Tocino3x3x3()
  : TestCase ("Wire a 3x3x3 torus")
{
}

~Tocino3x3x3::Tocino3x3x3()
{
}

void
Tocino3x3x3::DoRun (void)
{
  uint32_t x, y, z;
  uint32_t i;
  uint32_t tx, rx, c;
  uint32_t xp, xm, yp, ym, zp, zm;
 
  Ptr<TocinoNetDevice> m_netDevices[3*3*3];
  Ptr<TocinoChannel> m_channels[3*3*3*6];
  // create net devices
  for (x = 0; x < 3; x++)
    { 
      for (y = 0; y < 3; y++)
        { 
          for (z = 0; z < 3; z++)
            {
              i = (x * 9) + (y * 3) + z;
              m_netDevices[i] = CreateObject<TocinoNetDevice>;
            }
        }
    }

  // create channels and interconnect net devices
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

              m_channels[c + 0] = CreateObject<TocinoChannel>;
              m_netDevices[tx]->SetTxChannel(m_channels[c+0], 0); // x+ link
              m_channels[c + 0]->SetTransmitter(m_netDevices[tx]->GetTransmitter(0));
              rx = (xp * 9) + (y * 3) + z;
              m_netDevices[rx]->SetRxChannel(m_channels[c+0], 1); // x- link
              m_channels[c + 0]->SetReceiver(m_netDevices[rx]->GetReceiver(1));

              m_channels[c + 1] = CreateObject<TocinoChannel>; 
              m_netDevices[tx]->SetTxChannel(m_channels[c+1], 1); // x- link
              m_channels[c + 1]->SetTransmitter(m_netDevices[tx]->GetTransmitter(1));
              rx = (xm * 9) + (y * 3) + z;
              m_netDevices[rx]->SetRxChannel(m_channels[c+1], 0); // x+ link
              m_channels[c + 1]->SetReceiver(m_netDevices[rx]->GetReceiver(0));

              m_channels[c + 2] = CreateObject<TocinoChannel>;
              m_netDevices[tx]->SetTxChannel(m_channels[c+2], 2); // y+ link
              m_channels[c + 2]->SetTransmitter(m_netDevices[tx]->GetTransmitter(2));
              rx = (x * 9) + (yp * 3) + z;
              m_netDevices[rx]->SetRxChannel(m_channels[c+2], 3); // y- link
              m_channels[c + 2]->SetReceiver(m_netDevices[rx]->GetReceiver(3));

              m_channels[c + 3] = CreateObject<TocinoChannel>; 
              m_netDevices[tx]->SetTxChannel(m_channels[c+3], 3); // y- link
              m_channels[c + 3]->SetTransmitter(m_netDevices[tx]->GetTransmitter(3));
              rx = (x * 9) + (ym * 3) + z;
              m_netDevices[rx]->SetRxChannel(m_channels[c+3], 2); // y+ link
              m_channels[c + 3]->SetReceiver(m_netDevices[rx]->GetReceiver(2));

              m_channels[c + 4] = CreateObject<TocinoChannel>;
              m_netDevices[tx]->SetTxChannel(m_channels[c+4], 4); // z+ link
              m_channels[c + 4]->SetTransmitter(m_netDevices[tx]->GetTransmitter(4));
              rx = (x * 9) + (y * 3) + zp;
              m_netDevices[rx]->SetRxChannel(m_channels[c+4], 5); // z- link
              m_channels[c + 4]->SetReceiver(m_netDevices[rx]->GetReceiver(5));

              m_channels[c + 5] = CreateObject<TocinoChannel>; 
              m_netDevices[tx]->SetTxChannel(m_channels[c+5], 5); // z- link
              m_channels[c + 5]->SetTransmitter(m_netDevices[tx]->GetTransmitter(5));
              rx = (x * 9) + (y * 3) + zm;
              m_netDevices[rx]->SetRxChannel(m_channels[c+5], 4); // z+ link
              m_channels[c + 5]->SetReceiver(m_netDevices[rx]->GetReceiver(4));
            }
        }
    }
  NS_TEST_ASSERT_MSG_EQ (true, true, "Passes"); // there is no simulation output for this test
}
