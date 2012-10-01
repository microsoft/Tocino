/* -*- Mode:C++; c-file-style:"stroustrop"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"

class TocinoChannel;
class TocinoNetDevice;

class Tocino3x3x3 : public TestCase
{
public:
  Tocino3x3x3();
  virtual ~Tocino3x3x3();
private:
  virtual void DoRun (void);
  
  Ptr<TocinoChannel> TocinoLinkHelper(Ptr<TocinoNetDevice> tx_nd,
				      uint32_t tx_port,
				      Ptr<TocinoNetDevice> rx_nd,
				      uint32_t rx_port);
  std::vector<Ptr<TocinoChannel> > m_channels;
  std::vector<Ptr<TocinoNetDevice> > m_netDevices;
};
