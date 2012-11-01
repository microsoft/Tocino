/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_FLIT_HEADER_H__
#define __TEST_TOCINO_FLIT_HEADER_H__

#include "ns3/test.h"

namespace ns3
{

class TocinoFlitHeader;

class TestTocinoFlitHeader : public TestCase
{
    public:

    TestTocinoFlitHeader();
    virtual ~TestTocinoFlitHeader();

    private:

    bool TestSerializeHelper( const TocinoFlitHeader&, const uint8_t *, const unsigned );
    
    void TestDefaultLength();
    void TestSerializeLength();
    void TestDeserializeLength();
    
    void TestDefaultType();
    void TestSerializeType();
    void TestDeserializeType();
   
    void TestDefaultVirtualChannel();
    void TestSerializeVirtualChannel();
    void TestDeserializeVirtualChannel();

    void TestDefaultTail();
    void TestSerializeTail();
    void TestDeserializeTail();

    void TestDefaultHead();
    void TestSerializeHead();
    void TestDeserializeHead();
   
    void TestDefaultSource();
    void TestSerializeSource();
    void TestDeserializeSource();

    void TestDefaultDestination();
    void TestSerializeDestination();
    void TestDeserializeDestination();
    
    virtual void DoRun( void );
};

}

#endif // __TEST_TOCINO_FLIT_HEADER_H__
