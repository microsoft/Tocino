/* -*- Mode:C++; c-file-style:"stroustrup"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"
#include "ns3/packet.h"
#include "ns3/tocino-flow-control.h"
#include "ns3/tocino-flit-header.h"

#include "test-tocino-flow-control.h"

using namespace ns3;

TestTocinoFlowControl::TestTocinoFlowControl()
    : TestCase( "Tocino Flow Control Tests" )
{}

TestTocinoFlowControl::~TestTocinoFlowControl()
{}

void TestTocinoFlowControl::DoRun( void )
{
    TocinoFlitHeader h;
    const TocinoFlowControlState fcs( std::string( "0101" ) );

    Ptr<Packet> f = GetTocinoFlowControlFlit( fcs );
    
    NS_TEST_ASSERT_MSG_EQ( IsTocinoFlowControlFlit( f ), true, "Expected flow control flit." );
    NS_TEST_ASSERT_MSG_EQ( GetTocinoFlowControlState( f ), fcs, "Flit has unexpected FlowControlState." );

    f->PeekHeader( h );
    NS_TEST_ASSERT_MSG_EQ( h.GetType(), TocinoFlitHeader::LLC, "Flow control flit should have LLC type." );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "Flow control flit is not head?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "Flow control flit is not tail?" );
}
