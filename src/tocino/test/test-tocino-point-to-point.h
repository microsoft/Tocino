/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_POINT_TO_POINT_H__
#define __TEST_TOCINO_POINT_TO_POINT_H__

#include "ns3/test.h"
#include "ns3/ptr.h"

#include "ns3/tocino-address.h"
#include "ns3/tocino-net-device.h"

namespace ns3
{

class TestTocinoPointToPoint : public TestCase
{
public:
    TestTocinoPointToPoint();
    virtual ~TestTocinoPointToPoint();
private:
    Ptr<TocinoNetDevice> CreateNetDeviceHelper( const TocinoAddress& );
    void CreateChannelHelper( Ptr<TocinoNetDevice>, uint32_t, Ptr<TocinoNetDevice>, uint32_t );
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

}

#endif // __TEST_TOCINO_POINT_TO_POINT_H__
