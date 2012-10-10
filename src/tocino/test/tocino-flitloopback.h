/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_FLITLOOPBACK_H_
#define __TOCINO_FLITLOOPBACK_H_

#include "ns3/test.h"

using namespace ns3;

class TocinoFlitLoopback : public TestCase
{
public:
    TocinoFlitLoopback();
    virtual ~TocinoFlitLoopback();
private:
    void TestHelper( const unsigned, const unsigned );
    virtual void DoRun (void);
};

#endif // __TOCINO_FLITLOOPBACK_H_

