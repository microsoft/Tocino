/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3D_TORUS_H__
#define __TEST_TOCINO_3D_TORUS_H__

#include <stdint.h>
#include <vector>
#include <map>

#include "ns3/ptr.h"
#include "ns3/test.h"
#include "ns3/node-container.h"

#include "ns3/tocino-3d-torus-topology-helper.h"

namespace ns3
{

class TocinoAddress;

class TestTocino3DTorus : public TestCase
{
    public:

    TestTocino3DTorus( uint32_t radix, bool doWrap );
    virtual ~TestTocino3DTorus();

    private:
    
    const uint32_t RADIX;
    const uint32_t NODES;
   
    const bool m_doWrap;

    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address& );

    void CheckAllQuiet();
    void Reset();

    unsigned GetTotalCount() const;
    unsigned GetTotalBytes() const;

    TocinoAddress OppositeCorner( const uint8_t, const uint8_t, const uint8_t );
    int Middle() const;
    bool IsCenterNeighbor( const int x, const int y, const int z ) const;

    void TestCornerToCorner( const unsigned, const unsigned );
    void TestIncast( const unsigned, const unsigned );
    void TestAllToAll( const unsigned, const unsigned );
    void TestHelper();

    virtual void DoRun();

    NodeContainer m_machines;

    // 3D vector of NetDevices
    Tocino3DTorusNetDeviceContainer m_netDevices;
 
    // 2d map src*dst => unsigned
    typedef std::map< TocinoAddress, unsigned > TestMatrixRow;
    typedef std::map< TocinoAddress, TestMatrixRow  > TestMatrix;

    TestMatrix m_counts;
    TestMatrix m_bytes;
};

}

#endif // __TEST_TOCINO_3D_TORUS_H__
