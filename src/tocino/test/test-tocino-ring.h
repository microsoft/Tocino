/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_RING_H__
#define __TEST_TOCINO_RING_H__

#include "ns3/test.h"
#include "ns3/ptr.h"

#include "ns3/tocino-net-device.h"

using namespace ns3;

class TestTocinoRing : public TestCase
{
public:
    TestTocinoRing();
    virtual ~TestTocinoRing();
private:
    template <unsigned IDX>
    Ptr<TocinoNetDevice> CreateNetDeviceHelper( const TocinoAddress& );
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

#endif // __TEST_TOCINO_RING_H__
