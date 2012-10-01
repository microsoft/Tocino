/* -*- Mode:C++; c-file-style:"stroustrup"; indent-tabs-mode:nil; -*- */

#include "tocino-callbackqueue.h"
#include "test-flit-header.h"
#include "ns3/test.h"

using namespace ns3;

class TocinoTestSuite : public TestSuite
{
public:
  TocinoTestSuite ();
};

TocinoTestSuite::TocinoTestSuite ()
  : TestSuite ("tocino", UNIT)
{
  AddTestCase(new TocinoCallbackQueue);
  AddTestCase( new TestFlitHeader );
}

static TocinoTestSuite tocinoTestSuite;

