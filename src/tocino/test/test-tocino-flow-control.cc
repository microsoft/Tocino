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
    
    Ptr<Packet> xon = TocinoFlowControl::GetXONPacket();
    Ptr<Packet> xoff = TocinoFlowControl::GetXOFFPacket();
    
    xon->PeekHeader( h );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "XON packet is not head?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "XON packet is not tail?" );
    NS_TEST_ASSERT_MSG_EQ( h.GetType(), TocinoFlitHeader::LLC, "XON packet not LLC type?" );

    xoff->PeekHeader( h );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "XOFF packet is not head?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "XOFF packet is not tail?" );
    NS_TEST_ASSERT_MSG_EQ( h.GetType(), TocinoFlitHeader::LLC, "XOFF packet not LLC type?" );

    NS_TEST_ASSERT_MSG_EQ( TocinoFlowControl::IsXONPacket(xon), true, "XON packet is not XON packet?" );
    NS_TEST_ASSERT_MSG_EQ( TocinoFlowControl::IsXOFFPacket(xoff), true, "XON packet is not XON packet?" );
    NS_TEST_ASSERT_MSG_EQ( TocinoFlowControl::IsXONPacket(xoff), false, "XOFF packet is XON packet?" );
    NS_TEST_ASSERT_MSG_EQ( TocinoFlowControl::IsXOFFPacket(xon), false, "XON packet is XOFF packet?" );
}
