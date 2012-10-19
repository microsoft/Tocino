/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-3x3x3.h"
#include "tocino-callbackqueue.h"
#include "tocino-flitloopback.h"
#include "test-flit-header.h"
#include "test-flitter.h"
#include "test-point-to-point.h"

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
    AddTestCase( new TocinoCallbackQueue );
    AddTestCase( new TestFlitHeader );
    AddTestCase( new TestFlitter );
    AddTestCase( new TocinoFlitLoopback );
    AddTestCase( new TestPointToPoint );
    AddTestCase( new Tocino3x3x3 );
}

static TocinoTestSuite tocinoTestSuite;

