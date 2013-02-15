/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_3D_TORUS_TOPOLOGY_HELPER_H__
#define __TOCINO_3D_TORUS_TOPOLOGY_HELPER_H__

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"

#include "ns3/tocino-address.h"
#include "ns3/tocino-misc.h"

namespace ns3
{

class TocinoChannel;
class TocinoNetDevice;

// N.B.
// It's very useful to keep the net devices in a
// multi-dimensional container that reflects the
// topology.  The standard ns3 NetDeviceContainer
// is not a generic class, and thus cannot be
// made multi-dimensional.  We are forced to roll
// our own. -MAS

// A 3D vector of NetDevices
typedef std::vector<
    std::vector< 
        std::vector< 
            Ptr< TocinoNetDevice > 
        >
    >
> Tocino3DTorusNetDeviceContainer;

class Tocino3DTorusTopologyHelper
{
    public:

    Tocino3DTorusTopologyHelper( uint32_t );

    const uint32_t RADIX;
    const uint32_t NODES;
    
    uint32_t CoordinatesToIndex(
            const uint32_t,
            const uint32_t,
            const uint32_t ) const;

    uint32_t TocinoAddressToIndex( const TocinoAddress& ) const;
    TocinoAddress IndexToTocinoAddress( uint32_t ) const;
    
    Tocino3DTorusNetDeviceContainer Install( const NodeContainer& );
  
    uint32_t Middle() const;

    bool CrossesBisection( Ptr<TocinoChannel> ) const;
    
    void ReportBisectionBandwidth( 
            const Tocino3DTorusNetDeviceContainer&,
            const Time ) const;
    
    private:
   
    // ISSUE-REVIEW:
    // Better names? Make public?
    int Inc( const int ) const;
    int Dec( const int ) const;
};

}

#endif // __TOCINO_3D_TORUS_TOPOLOGY_HELPER_H__
