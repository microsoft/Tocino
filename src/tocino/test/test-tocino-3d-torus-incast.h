/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3D_TORUS_INCAST_H__
#define __TEST_TOCINO_3D_TORUS_INCAST_H__

#include <stdint.h>

#include "test-tocino-3d-torus.h"

namespace ns3
{

class TestTocino3DTorusIncast : public TestTocino3DTorus
{
    public:

    TestTocino3DTorusIncast( uint32_t radix, bool doWrap );

    private:
    
    bool IsCenter( const uint32_t ) const;
    bool IsCenterNeighbor( const uint32_t ) const;

    void TestHelper( const Time, const unsigned );

    virtual void DoRun();
};

}

#endif // __TEST_TOCINO_3D_TORUS_INCAST_H__
