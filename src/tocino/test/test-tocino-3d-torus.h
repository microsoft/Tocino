/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3D_TORUS_H__
#define __TEST_TOCINO_3D_TORUS_H__

#include <stdint.h>
#include <vector>

#include "ns3/test.h"

#include "ns3/tocino-3d-torus-topology-helper.h"
#include "ns3/tocino-traffic-matrix-application.h"

namespace ns3
{

class TestTocino3DTorus : public TestCase
{
    public:

    TestTocino3DTorus( uint32_t, bool, bool, std::string );

    protected:
    
    const uint32_t RADIX;
    const uint32_t NODES;
   
    const bool m_doWrap;
    const bool m_doVLB;

    void CheckAllQuiet( const Tocino3DTorusNetDeviceContainer& );
   
    typedef std::vector< Ptr<TocinoTrafficMatrixApplication > >
        AppVector;

    uint32_t GetTotalPacketsSent( const AppVector& ) const;

    Tocino3DTorusTopologyHelper m_helper;
    TocinoTrafficMatrix m_trafficMatrix;
};

}

#endif // __TEST_TOCINO_3D_TORUS_H__
