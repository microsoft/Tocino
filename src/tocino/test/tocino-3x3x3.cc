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

  std::vector<Ptr<TocinoChannel>> m_channel;
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
  // TBD
}
