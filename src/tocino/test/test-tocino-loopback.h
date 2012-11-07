/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_LOOPBACK_H_
#define __TEST_TOCINO_LOOPBACK_H_

#include "ns3/test.h"

namespace ns3
{

class TestTocinoLoopback : public TestCase
{
public:
    TestTocinoLoopback();
    virtual ~TestTocinoLoopback();
private:
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

}

#endif // __TEST_TOCINO_LOOPBACK_H_
