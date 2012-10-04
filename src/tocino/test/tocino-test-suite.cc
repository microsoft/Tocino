/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-3x3x3.h"
#include "tocino-callbackqueue.h"
#include "tocino-flitloopback.h"
#include "test-flit-header.h"
#include "test-flitter-deflitter.h"

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
    AddTestCase( new TocinoFlitLoopback );
    AddTestCase( new Tocino3x3x3 );
    AddTestCase( new TocinoCallbackQueue );
    AddTestCase( new TestFlitHeader );
    AddTestCase( new TestFlitter );
    AddTestCase( new TestDeflitter );
}

static TocinoTestSuite tocinoTestSuite;

