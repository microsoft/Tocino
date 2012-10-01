/* -*- Mode:C++; c-file-style:"stroustrop"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_3X3X3_H__
#define __TOCINO_3X3X3_H__

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"
#include "ns3/test.h"

#include "ns3/tocino-channel.h"
#include "ns3/tocino-net-device.h"

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
#endif // __TOCINO_3X3X3_H__
