/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/tocino-net-device.h"

#include "test-tocino-3d-torus.h"

using namespace ns3;

TestTocino3DTorus::TestTocino3DTorus( uint32_t radix, bool doWrap, std::string name )
    : TestCase( doWrap ?
            "Test a 3D torus" + name  :
            "Test a 3D mesh" + name )
    , RADIX( radix )
    , NODES( radix * radix * radix )
    , m_doWrap( doWrap )
    , m_helper( radix )
{}

void
TestTocino3DTorus::CheckAllQuiet(
        const Tocino3DTorusNetDeviceContainer& netDevices )
{
    bool aq = true;

    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                aq &= netDevices[x][y][z]->AllQuiet();
            }
        }
    }
  
    if( aq ) return;

    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                netDevices[x][y][z]->DumpState();
            }
        }
    }

    NS_TEST_ASSERT_MSG_EQ( aq, true, "not all quiet?" );
}

uint32_t
TestTocino3DTorus::GetTotalPacketsSent(
        const AppVector& av ) const
{
    uint32_t total = 0;

    AppVector::const_iterator ap;

    for( ap = av.begin(); ap != av.end(); ++ap )
    {
        total += (*ap)->GetPacketsSent();
    }
   
    return total;
}
