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
#include "test-tocino-3d-torus-corner-to-corner.h"
#include "test-tocino-3d-torus-incast.h"
#include "test-tocino-3d-torus-all-to-all.h"

#include "ns3/log.h"
#include "ns3/test.h"

using namespace ns3;

class TocinoTestSuite : public TestSuite
{
public:
    TocinoTestSuite ();
private:
    void Add3DTorusTestCases( bool, bool );
};

void
TocinoTestSuite::Add3DTorusTestCases( bool doTorus, bool doVLB )
{
    const uint32_t RADIX = 3;

    AddTestCase( new TestTocino3DTorusCornerToCorner( RADIX, doTorus, doVLB ) );
    AddTestCase( new TestTocino3DTorusIncast( RADIX, doTorus, doVLB ) );
    AddTestCase( new TestTocino3DTorusAllToAll( RADIX, doTorus, doVLB ) );
}

TocinoTestSuite::TocinoTestSuite ()
    : TestSuite ("tocino", UNIT)
{
    AddTestCase( new TestTocinoCallbackQueue );
    AddTestCase( new TestTocinoFlitHeader );
    AddTestCase( new TestTocinoFlitter );
    AddTestCase( new TestTocinoFlowControl );
    AddTestCase( new TestTocinoLoopback );
    AddTestCase( new TestTocinoPointToPoint );
    AddTestCase( new TestTocinoMultihop );
    AddTestCase( new TestTocinoRing );
    AddTestCase( new TestTocinoDeadlock );
    Add3DTorusTestCases( false, false );
    Add3DTorusTestCases( true, false );
    Add3DTorusTestCases( false, true );
    Add3DTorusTestCases( true, true );
}

static TocinoTestSuite tocinoTestSuite;

