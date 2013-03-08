/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3D_TORUS_CORNER_TO_CORNER_H__
#define __TEST_TOCINO_3D_TORUS_CORNER_TO_CORNER_H__

#include <stdint.h>

#include "test-tocino-3d-torus.h"

namespace ns3
{

class TocinoAddress;

class TestTocino3DTorusCornerToCorner : public TestTocino3DTorus
{
    public:

    TestTocino3DTorusCornerToCorner( uint32_t radix, bool doWrap, bool doVLB );

    private:
    
    const uint32_t MAX_COORD;
   
    bool IsCorner( uint32_t ) const;
    uint32_t GetOppositeCorner( const uint8_t ) const;
    
    void TestHelper( const Time, const unsigned );

    virtual void DoRun();
};

}

#endif // __TEST_TOCINO_3D_TORUS_CORNER_TO_CORNER_H__
