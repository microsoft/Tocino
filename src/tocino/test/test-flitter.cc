/* -*- Mode:C++; c-file-style:"stroustrup"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

#include "test-flitter.h"
#include "ns3/tocino-net-device.h"
#include "ns3/tocino-flit-header.h"

using namespace ns3;

TestFlitter::TestFlitter()
    : TestCase( "Tocino Flitter Tests" )
{}

TestFlitter::~TestFlitter()
{}

void TestFlitter::TestEmpty()
{
    Ptr<Packet> p = Create<Packet>( 0 );
    std::vector< Ptr<Packet> > flits;

    flits = TocinoNetDevice::Flitter( p, 0, 1 );

    NS_TEST_ASSERT_MSG_EQ( flits.size(), 1, "Empty packet should result in one flit" );
}

void TestFlitter::TestOneFlit( const unsigned LEN )
{
    const TocinoAddress SRC(0);
    const TocinoAddress DST(1);

    Ptr<Packet> p = Create<Packet>( LEN );
    std::vector< Ptr<Packet> > flits;

    flits = TocinoNetDevice::Flitter( p, SRC, DST );

    NS_TEST_ASSERT_MSG_EQ( flits.size(), 1, "Incorrect number of flits" );

    TocinoFlitHeader h;
    flits[0]->PeekHeader( h );

    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), LEN, "Flit has wrong length" );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "Head flit missing head flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "Tail flit missing tail flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.GetSource(), SRC, "Flit has incorrect source" );
    NS_TEST_ASSERT_MSG_EQ( h.GetDestination(), DST, "Flit has incorrect destination" );
}

void TestFlitter::TestTwoFlits( const unsigned TAIL_LEN )
{
    const TocinoAddress SRC(2);
    const TocinoAddress DST(3);

    const unsigned HEAD_LEN = TocinoFlitHeader::MAX_PAYLOAD_HEAD;
    const unsigned LEN = HEAD_LEN + TAIL_LEN;
    
    Ptr<Packet> p = Create<Packet>( LEN );
    std::vector< Ptr<Packet> > flits;
    
    flits = TocinoNetDevice::Flitter( p, SRC, DST );

    NS_TEST_ASSERT_MSG_EQ( flits.size(), 2, "Incorrect number of flits" );

    TocinoFlitHeader h;

    // interrogate head flit
    flits[0]->PeekHeader( h );

    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), HEAD_LEN, "Head flit has wrong length" );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "Head flit missing head flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), false, "Head flit has tail flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.GetSource(), SRC, "Flit has incorrect source" );
    NS_TEST_ASSERT_MSG_EQ( h.GetDestination(), DST, "Flit has incorrect destination" );
    
    // interrogate tail flit 
    flits[1]->PeekHeader( h );
    
    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), TAIL_LEN, "Tail flit has wrong length" );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), false, "Tail flit has head flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "Tail flit missing tail flag?" );
}

void TestFlitter::TestThreeFlits( const unsigned TAIL_LEN )
{
    const TocinoAddress SRC(4);
    const TocinoAddress DST(5);

    const unsigned HEAD_LEN = TocinoFlitHeader::MAX_PAYLOAD_HEAD;
    const unsigned BODY_LEN = TocinoFlitHeader::MAX_PAYLOAD_OTHER;
    const unsigned LEN = HEAD_LEN + BODY_LEN + TAIL_LEN;

    Ptr<Packet> p = Create<Packet>( LEN );
    std::vector< Ptr<Packet> > flits;

    flits = TocinoNetDevice::Flitter( p, SRC, DST );

    NS_TEST_ASSERT_MSG_EQ( flits.size(), 3, "Incorrect number of flits" );

    TocinoFlitHeader h;

    // interrogate head flit
    flits[0]->PeekHeader( h );

    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), HEAD_LEN, "Head flit has wrong length" );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "Head flit missing head flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), false, "Head flit has tail flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.GetSource(), SRC, "Flit has incorrect source" );
    NS_TEST_ASSERT_MSG_EQ( h.GetDestination(), DST, "Flit has incorrect destination" );
    
    // interrogate body flit 
    flits[1]->PeekHeader( h );
    
    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), BODY_LEN, "Body flit has wrong length" );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), false, "Body flit has head flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), false, "Body flit has tail flag?" );
    
    // interrogate tail flit 
    flits[2]->PeekHeader( h );
    
    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), TAIL_LEN, "Tail flit has wrong length" );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), false, "Tail flit has head flag?" );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "Tail flit missing tail flag?" );
}

void TestFlitter::DoRun( void )
{
    TestEmpty();

    TestOneFlit( 0 ); // similar to TestEmpty()
    TestOneFlit( 1 );
    TestOneFlit( TocinoFlitHeader::MAX_PAYLOAD_HEAD - 1 );
    
    TestTwoFlits( 1 );
    TestTwoFlits( TocinoFlitHeader::MAX_PAYLOAD_HEAD - 1 );

    TestThreeFlits( 1 );
    TestThreeFlits( TocinoFlitHeader::MAX_PAYLOAD_HEAD - 1 );
}
