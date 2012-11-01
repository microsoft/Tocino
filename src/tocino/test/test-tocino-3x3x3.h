/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3X3X3_H__
#define __TEST_TOCINO_3X3X3_H__

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"
#include "ns3/test.h"

#include "ns3/tocino-channel.h"
#include "ns3/tocino-net-device.h"

namespace ns3
{

class TestTocino3x3x3 : public TestCase
{
public:
    TestTocino3x3x3();
    virtual ~TestTocino3x3x3();
private:
    virtual void DoRun (void);
  
    Ptr<TocinoChannel>
        TocinoLinkHelper(Ptr<TocinoNetDevice> tx_nd,
                                        uint32_t tx_port,
                                        Ptr<TocinoNetDevice> rx_nd,
                                        uint32_t rx_port);

    std::vector<Ptr<TocinoChannel> > m_channels;
    std::vector<Ptr<TocinoNetDevice> > m_netDevices;
};

}

#endif // __TEST_TOCINO_3X3X3_H__
