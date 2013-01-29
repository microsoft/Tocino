/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "test-tocino-callbackqueue.h"
#include "test-tocino-flit-header.h"
#include "test-tocino-flitter.h"
#include "test-tocino-flow-control.h"
#include "test-tocino-loopback.h"
#include "test-tocino-point-to-point.h"
#include "test-tocino-multihop.h"
#include "test-tocino-ring.h"
#include "test-tocino-deadlock.h"
#include "test-tocino-3d-torus.h"

#include "ns3/log.h"
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
    //AddTestCase( new TestTocinoCallbackQueue );
    //AddTestCase( new TestTocinoFlitHeader );
    //AddTestCase( new TestTocinoFlitter );
    //AddTestCase( new TestTocinoFlowControl );
    //AddTestCase( new TestTocinoLoopback );
    //AddTestCase( new TestTocinoPointToPoint );
    //AddTestCase( new TestTocinoMultihop );
    //AddTestCase( new TestTocinoRing );
    //AddTestCase( new TestTocinoDeadlock );
   
    // Topologically a torus, however use default mesh routing
    AddTestCase( new TestTocino3DTorus( 3, false ) );
   
    // Route around the torus edges this time
    AddTestCase( new TestTocino3DTorus( 3, true ) );
}

static TocinoTestSuite tocinoTestSuite;

