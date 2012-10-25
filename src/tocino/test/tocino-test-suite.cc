/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "test-tocino-callbackqueue.h"
#include "test-tocino-flit-header.h"
#include "test-tocino-flitter.h"
#include "test-tocino-flitloopback.h"
#include "test-tocino-point-to-point.h"
#include "test-tocino-ring.h"
#include "test-tocino-3x3x3.h"

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
    AddTestCase( new TestTocinoCallbackQueue );
    AddTestCase( new TestTocinoFlitHeader );
    AddTestCase( new TestTocinoFlitter );
    AddTestCase( new TestTocinoFlitLoopback );
    AddTestCase( new TestTocinoPointToPoint );
    AddTestCase( new TestTocinoRing );
    AddTestCase( new TestTocino3x3x3 );
}

static TocinoTestSuite tocinoTestSuite;

