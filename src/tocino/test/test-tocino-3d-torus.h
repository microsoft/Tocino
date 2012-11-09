/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TEST_TOCINO_3D_TORUS_H__
#define __TEST_TOCINO_3D_TORUS_H__

#include <stdint.h>
#include <vector>
#include <map>

#include "ns3/ptr.h"
#include "ns3/test.h"

namespace ns3
{

class TocinoAddress;

class TestTocino3DTorus : public TestCase
{
public:
    TestTocino3DTorus();
    virtual ~TestTocino3DTorus();
private:
    int m_radix;

    int Inc( const int i ) const;
    int Dec( const int i ) const;

    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address& );

    void Initialize();
    
    void Reset();

    unsigned GetTotalCount() const;
    unsigned GetTotalBytes() const;

    void RunOneTest( const unsigned, const unsigned, const TocinoAddress&, const TocinoAddress& );
    void TestHelper( const unsigned, const unsigned );

    TocinoAddress OppositeCorner( const TocinoAddress& );

    virtual void DoRun();
 
    // 3D vector of NetDevices
    std::vector< std::vector< std::vector< 
        Ptr< TocinoNetDevice > > > > m_netDevices;
  
    // 2d map src*dst => unsigned
    typedef std::map< TocinoAddress, unsigned > TestMatrixRow;
    typedef std::map< TocinoAddress, TestMatrixRow  > TestMatrix;

    TestMatrix m_counts;
    TestMatrix m_bytes;
};

}

#endif // __TEST_TOCINO_3D_TORUS_H__
