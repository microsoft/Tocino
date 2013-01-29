/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-3d-torus-topology-helper.h"
#include "tocino-helper.h"

#include "ns3/tocino-net-device.h"

namespace ns3
{

Tocino3DTorusTopologyHelper::Tocino3DTorusTopologyHelper( uint32_t radix )
    : RADIX( radix )
    , NODES( radix * radix * radix )
{}

int
Tocino3DTorusTopologyHelper::Inc( const int i ) const
{
    return ( (i + RADIX) + 1 ) % RADIX;
}

int
Tocino3DTorusTopologyHelper::Dec( const int i ) const
{
    return ( (i + RADIX) - 1 ) % RADIX;
}

uint32_t 
Tocino3DTorusTopologyHelper::CoordinatesToIndex(
        const uint32_t x,
        const uint32_t y,
        const uint32_t z ) const
{
    return x + (y*RADIX) + (z*RADIX*RADIX);
}

uint32_t 
Tocino3DTorusTopologyHelper::TocinoAddressToIndex( const TocinoAddress& ta ) const
{
    return CoordinatesToIndex( ta.GetX(), ta.GetY(), ta.GetZ() ); 
}

TocinoAddress 
Tocino3DTorusTopologyHelper::IndexToTocinoAddress( uint32_t idx ) const
{
    uint32_t rem = idx;
    
    const uint32_t z = rem / (RADIX * RADIX);
    rem = rem % (RADIX * RADIX);
    
    const uint32_t y = rem / RADIX;
    rem = rem % RADIX;
    
    const uint32_t x = rem;

    return TocinoAddress( x, y, z );
}

Tocino3DTorusNetDeviceContainer
Tocino3DTorusTopologyHelper::Install( const NodeContainer& nc )
{
    NS_ASSERT( nc.GetN() == NODES );
    
    Tocino3DTorusNetDeviceContainer netDevices;
   
    // Create the net devices
    netDevices.resize( RADIX );
    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        netDevices[x].resize( RADIX );
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            netDevices[x][y].resize( RADIX );
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                TocinoAddress ta( x, y, z );
                Ptr<TocinoNetDevice> tnd = CreateObject<TocinoNetDevice>();

                netDevices[x][y][z] = tnd;
                
                tnd->Initialize();
                tnd->SetAddress( ta );
            }
        }
    }

    // Create channels and interconnect net devices
    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                Ptr<TocinoNetDevice> cur = netDevices[x][y][z];

                TocinoChannelHelper( cur, 0, netDevices[ Inc(x) ][y][z], 1 ); // x+
                TocinoChannelHelper( cur, 1, netDevices[ Dec(x) ][y][z], 0 ); // x-

                TocinoChannelHelper( cur, 2, netDevices[x][ Inc(y) ][z], 3 ); // y+
                TocinoChannelHelper( cur, 3, netDevices[x][ Dec(y) ][z], 2 ); // y-

                TocinoChannelHelper( cur, 4, netDevices[x][y][ Inc(z) ], 5 ); // z+
                TocinoChannelHelper( cur, 5, netDevices[x][y][ Dec(z) ], 4 ); // z-

                // Attach net device to the correct node
                uint32_t idx = CoordinatesToIndex( x, y, z );

                Ptr<Node> node = nc.Get( idx );
                NS_ASSERT( node != NULL );

                node->AddDevice( cur );
            }
        }
    }

    return netDevices;
}

}
