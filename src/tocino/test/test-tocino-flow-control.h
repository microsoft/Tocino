/* -*- Mode:C++; c-file-style:"stroustrup"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_FLOW_CONTROL_H__
#define __TEST_TOCINO_FLOW_CONTROL_H__

#include "ns3/test.h"

namespace ns3
{

class TestTocinoFlowControl : public TestCase
{
    public:

    TestTocinoFlowControl();
    virtual ~TestTocinoFlowControl();

    private:

    virtual void DoRun( void );
};

}

#endif // __TEST_TOCINO_FLOW_CONTROL_H__
