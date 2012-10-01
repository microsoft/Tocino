/* -*- Mode:C++; c-file-style:"stroustrup"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"

#include "ns3/tocino-flit-header.h"

#include "test-flit-header.h"

using namespace ns3;

TestFlitHeader::TestFlitHeader()
    : TestCase( "Tocino Flit Header Tests" )
{}

TestFlitHeader::~TestFlitHeader()
{}

bool TestFlitHeader::TestSerializeHelper( const TocinoFlitHeader &h, const uint8_t *data, const unsigned SIZE )
{
    if( h.GetSerializedSize() != SIZE )
    {
        return false;
    }

    Buffer b;
    b.AddAtStart( SIZE );
    h.Serialize( b.Begin() );

    uint8_t *bufData = new uint8_t[SIZE]; //unique_ptr?
    b.CopyData( bufData, SIZE );

    int mc = memcmp( bufData, data, SIZE );

    delete [] bufData;

    if( mc != 0 )
    {
        return false;
    }

    return true;
}

void TestFlitHeader::TestDefaultLength()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), 0, "New header has incorrect length?" );
}

void TestFlitHeader::TestSerializeLength()
{
    TocinoFlitHeader h;
    h.SetLength( ~0 );

    uint8_t expected[] = { 0x00, 0x3F };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetLength" );
}

void TestFlitHeader::TestDeserializeLength()
{
    uint8_t input[] = { 0x00, 0x3F };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.GetLength(), 0x3F, "Deserialized header has incorrect length?" );
}

void TestFlitHeader::TestDefaultType()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.GetType(), TocinoFlitHeader::INVALID, "New header has incorrect type?" );
}

void TestFlitHeader::TestSerializeType()
{
    TocinoFlitHeader h;

    h.SetType( TocinoFlitHeader::MAX_TYPE );

    uint8_t expected[] = { TocinoFlitHeader::MAX_TYPE << 4, 0x00 };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetType" );
}

void TestFlitHeader::TestDeserializeType()
{
    uint8_t input[] = { TocinoFlitHeader::MAX_TYPE << 4, 0x00 };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.GetType(), TocinoFlitHeader::MAX_TYPE, "Deserialized header has incorrect type?" );
}

void TestFlitHeader::TestDefaultVirtualChannel()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.GetVirtualChannel(), 0, "New header has non-zero virtual channel?" );
}

void TestFlitHeader::TestSerializeVirtualChannel()
{
    TocinoFlitHeader h;

    h.SetVirtualChannel( ~0 );

    uint8_t expected[] = { 0x0F, 0x00 };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetVirtualChannel" );
}

void TestFlitHeader::TestDeserializeVirtualChannel()
{
    uint8_t input[] = { 0x0F, 0x00 };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.GetVirtualChannel(), 0xF, "Deserialized header has incorrect virtual channel?" );
}

void TestFlitHeader::TestDefaultTail()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), false, "New header is tail?" );
}

void TestFlitHeader::TestSerializeTail()
{
    TocinoFlitHeader h;

    h.SetTail();

    uint8_t expected[] = { 0x00, 0x40 };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetTail" );
}

void TestFlitHeader::TestDeserializeTail()
{
    uint8_t input[] = { 0x00, 0x40 };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.IsTail(), true, "Deserialized header not tail?" );
}

void TestFlitHeader::TestDefaultHead()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), false, "New header is head?" );
} 

void TestFlitHeader::TestSerializeHead()
{
    TocinoFlitHeader h;

    h.SetHead();

    uint8_t expected[] = 
    {
        0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetHead" );
}

void TestFlitHeader::TestDeserializeHead()
{
    uint8_t input[] = 
    {
        0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.IsHead(), true, "Deserialized header not head?" );
}

void TestFlitHeader::TestDefaultSource()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.GetSource(), TocinoAddress( 0 ), "New header has non-zero source?" );
}

void TestFlitHeader::TestSerializeSource()
{
    TocinoFlitHeader h;

    h.SetHead();
    h.SetSource( ~0 );

    uint8_t expected[] = 
    {
        0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00
    };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetSource" );
}

void TestFlitHeader::TestDeserializeSource()
{
    uint8_t input[] = 
    {
        0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00
    };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.GetSource(), TocinoAddress( ~0 ), "Deserialized header has incorrect source?" );
}

void TestFlitHeader::TestDefaultDestination()
{
    TocinoFlitHeader h;
    NS_TEST_ASSERT_MSG_EQ( h.GetDestination(), TocinoAddress( 0 ), "New header has non-zero dest?" );
}

void TestFlitHeader::TestSerializeDestination()
{
    TocinoFlitHeader h;

    h.SetHead();
    h.SetDestination( ~0 );

    uint8_t expected[] = 
    {
        0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF
    };

    bool pass = TestSerializeHelper( h, expected, sizeof( expected ) );
    NS_TEST_ASSERT_MSG_EQ( pass, true, "Bad serialize after SetDestination" );
}

void TestFlitHeader::TestDeserializeDestination()
{
    uint8_t input[] = 
    {
        0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF
    };

    Buffer b;
    b.AddAtStart( sizeof( input ) ); 
    b.Begin().Write( input, sizeof( input ) );

    TocinoFlitHeader h;
    h.Deserialize( b.Begin() );
    NS_TEST_ASSERT_MSG_EQ( h.GetDestination(), TocinoAddress( ~0 ), "Deserialized header has incorrect destination?" );
}

void TestFlitHeader::DoRun( void )
{
    TestDefaultLength();
    TestSerializeLength();
    TestDeserializeLength();

    TestDefaultType();
    TestSerializeType();
    TestDeserializeType();

    TestDefaultVirtualChannel();
    TestSerializeVirtualChannel();
    TestDeserializeVirtualChannel();

    TestDefaultTail();
    TestSerializeTail();
    TestDeserializeTail();

    TestDefaultHead();
    TestSerializeHead();
    TestDeserializeHead();

    TestDefaultSource();
    TestSerializeSource();
    TestDeserializeSource();

    TestDefaultDestination();
    TestSerializeDestination();
    TestDeserializeDestination();
}
