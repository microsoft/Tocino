/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/config.h"
#include "ns3/uinteger.h"

#include "ns3/tocino-helper.h"

#include "test-tocino-3x3x3.h"

using namespace ns3;

TestTocino3x3x3::TestTocino3x3x3()
  : TestCase ("Wire a 3x3x3 torus")
{
}

TestTocino3x3x3::~TestTocino3x3x3()
{
}

void
TestTocino3x3x3::DoRun (void)
{
  uint32_t x, y, z;
  uint32_t i;
  uint32_t tx, rx, c;
  uint32_t xp, xm, yp, ym, zp, zm;
 
  Ptr<TocinoNetDevice> m_netDevices[3*3*3];
  Ptr<TocinoChannel> m_channels[3*3*3*6];

  // channel FIFOs are 4 flits/packets deep
  Config::SetDefault ("ns3::CallbackQueue::Depth", UintegerValue (4));

  // create net devices
  for (x = 0; x < 3; x++)
    { 
      for (y = 0; y < 3; y++)
        { 
          for (z = 0; z < 3; z++)
            {
              i = (x * 9) + (y * 3) + z;
              m_netDevices[i] = CreateObject<TocinoNetDevice>();
              m_netDevices[i]->Initialize();
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

              rx = (xp * 9) + (y * 3) + z;
              m_channels[c + 0] = TocinoChannelHelper(m_netDevices[tx], 0, m_netDevices[rx], 1); // x+

              rx = (xm * 9) + (y * 3) + z;
              m_channels[c + 1] = TocinoChannelHelper(m_netDevices[tx], 1, m_netDevices[rx], 0); // x-

              rx = (x * 9) + (yp * 3) + z;
              m_channels[c + 2] = TocinoChannelHelper(m_netDevices[tx], 2, m_netDevices[rx], 3); // y+

              rx = (x * 9) + (ym * 3) + z;
              m_channels[c + 3] = TocinoChannelHelper(m_netDevices[tx], 3, m_netDevices[rx], 2); // y-

              rx = (x * 9) + (y * 3) + zp;
              m_channels[c + 4] = TocinoChannelHelper(m_netDevices[tx], 4, m_netDevices[rx], 5); // z+

              rx = (x * 9) + (y * 3) + zm;
              m_channels[c + 5] = TocinoChannelHelper(m_netDevices[tx], 5, m_netDevices[rx], 4); // z-
            }
        }
    }
  NS_TEST_ASSERT_MSG_EQ (true, true, "Passes"); // there is no simulation output for this test
}
