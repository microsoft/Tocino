/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3D_TORUS_ALL_TO_ALL_H__
#define __TEST_TOCINO_3D_TORUS_ALL_TO_ALL_H__

#include <stdint.h>

#include "test-tocino-3d-torus.h"

namespace ns3
{

class TestTocino3DTorusAllToAll : public TestTocino3DTorus
{
    public:

    TestTocino3DTorusAllToAll( uint32_t radix, bool doWrap, bool doVLB );

    private:
    
    void TestHelper( const Time, const unsigned );

    virtual void DoRun();
};

}

#endif // __TEST_TOCINO_3D_TORUS_ALL_TO_ALL_H__
