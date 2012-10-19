/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TEST_POINT_TO_POINT_H__
#define __TOCINO_TEST_POINT_TO_POINT_H__

#include "ns3/test.h"

using namespace ns3;

class TestPointToPoint : public TestCase
{
public:
    TestPointToPoint();
    virtual ~TestPointToPoint();
private:
    Ptr<TocinoNetDevice> CreateNetDeviceHelper( const TocinoAddress& );
    void CreateChannelHelper( Ptr<TocinoNetDevice>, uint32_t, Ptr<TocinoNetDevice>, uint32_t );
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

#endif // __TOCINO_TEST_POINT_TO_POINT_H__
