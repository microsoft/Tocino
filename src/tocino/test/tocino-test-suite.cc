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

    AddTestCase( new TestTocino3DTorusCornerToCorner( RADIX, doTorus, doVLB ), QUICK );
    AddTestCase( new TestTocino3DTorusIncast( RADIX, doTorus, doVLB ), QUICK );
    AddTestCase( new TestTocino3DTorusAllToAll( RADIX, doTorus, doVLB ), QUICK );
}

TocinoTestSuite::TocinoTestSuite ()
    : TestSuite ("tocino", UNIT)
{
    AddTestCase( new TestTocinoCallbackQueue, QUICK );
    AddTestCase( new TestTocinoFlitHeader, QUICK );
    AddTestCase( new TestTocinoFlitter, QUICK );
    AddTestCase( new TestTocinoFlowControl, QUICK );
    AddTestCase( new TestTocinoLoopback, QUICK );
    AddTestCase( new TestTocinoPointToPoint, QUICK );
    AddTestCase( new TestTocinoMultihop, QUICK );
    AddTestCase( new TestTocinoRing, QUICK );
    AddTestCase( new TestTocinoDeadlock, QUICK );
    Add3DTorusTestCases( false, false );
    Add3DTorusTestCases( true, false );
    Add3DTorusTestCases( false, true );
    Add3DTorusTestCases( true, true );
}

static TocinoTestSuite tocinoTestSuite;

