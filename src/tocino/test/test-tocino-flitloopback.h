/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_FLITLOOPBACK_H_
#define __TEST_TOCINO_FLITLOOPBACK_H_

#include "ns3/test.h"

using namespace ns3;

class TestTocinoFlitLoopback : public TestCase
{
public:
    TestTocinoFlitLoopback();
    virtual ~TestTocinoFlitLoopback();
private:
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

#endif // __TEST_TOCINO_FLITLOOPBACK_H_

