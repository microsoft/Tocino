/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-channel.h"
#include "ns3/tocino-flit-header.h"

using namespace ns3;

class TestFlitter : public TestCase
{
    public:

    TestFlitter()
        : TestCase( "Tocino Flitter Test 1" )
    {}

    virtual ~TestFlitter()
    {}

    private:
    
    virtual void DoRun( void )
    {
        TocinoFlitHeader h;
        
        // A wide variety of test macros are available in src/core/test.h

        uint32_t size = h.GetSerializedSize();
        NS_TEST_ASSERT_MSG_EQ( size, 0, "Empty header has non-zero size");
    }
};

class TocinoTestSuite : public TestSuite
{
    public:
    
    TocinoTestSuite()
        : TestSuite( "tocino", UNIT )
    {
        AddTestCase( new TestFlitter );
    }
};

static TocinoTestSuite tocinoTestSuite;
