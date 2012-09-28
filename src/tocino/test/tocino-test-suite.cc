/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"

#include "test-flit-header.h"

using namespace ns3;

class TocinoTestSuite : public TestSuite
{
    public:
    
    TocinoTestSuite()
        : TestSuite( "tocino", UNIT )
    {
        AddTestCase( new TestFlitHeader );
    }
};

static TocinoTestSuite tocinoTestSuite;
