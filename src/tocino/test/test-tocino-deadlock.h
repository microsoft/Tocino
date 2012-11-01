/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_DEADLOCK_H__
#define __TEST_TOCINO_DEADLOCK_H__

#include "ns3/test.h"
#include "ns3/ptr.h"

#include "ns3/tocino-net-device.h"

namespace ns3
{

class TestTocinoDeadlock : public TestCase
{
public:
    TestTocinoDeadlock();
    virtual ~TestTocinoDeadlock();
private:
    Ptr<TocinoNetDevice> CreateNetDeviceHelper( const TocinoAddress& );
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

}

#endif // __TEST_TOCINO_DEADLOCK_H__
