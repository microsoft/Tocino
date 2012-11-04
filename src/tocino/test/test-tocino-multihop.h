/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_MULTIHOP_H__
#define __TEST_TOCINO_MULTIHOP_H__

#include "ns3/test.h"
#include "ns3/ptr.h"

#include "ns3/tocino-address.h"
#include "ns3/tocino-net-device.h"

namespace ns3
{

class TestTocinoMultihop : public TestCase
{
public:
    TestTocinoMultihop();
    virtual ~TestTocinoMultihop();
private:
    Ptr<TocinoNetDevice> CreateNetDeviceHelper( const TocinoAddress& );
    void CreateChannelHelper( Ptr<TocinoNetDevice>, uint32_t, Ptr<TocinoNetDevice>, uint32_t );
    void TestHelper( const unsigned );
    virtual void DoRun (void);
};

}

#endif // __TEST_TOCINO_MULTIHOP_H__
