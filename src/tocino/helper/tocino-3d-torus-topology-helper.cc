/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include <iostream>

#include "ns3/log.h"

#include "tocino-3d-torus-topology-helper.h"
#include "tocino-helper.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-channel.h"
#include "ns3/tocino-3d-torus-topology.h"

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

                TocinoChannelHelper(
                        cur,
                        Tocino3DTorusTopology::X_POS, 
                        netDevices[ Inc(x) ][y][z],
                        Tocino3DTorusTopology::X_NEG );

                TocinoChannelHelper(
                        cur,
                        Tocino3DTorusTopology::X_NEG,
                        netDevices[ Dec(x) ][y][z],
                        Tocino3DTorusTopology::X_POS );

                TocinoChannelHelper(
                        cur,
                        Tocino3DTorusTopology::Y_POS,
                        netDevices[x][ Inc(y) ][z],
                        Tocino3DTorusTopology::Y_NEG );

                TocinoChannelHelper(
                        cur,
                        Tocino3DTorusTopology::Y_NEG,
                        netDevices[x][ Dec(y) ][z],
                        Tocino3DTorusTopology::Y_POS );

                TocinoChannelHelper(
                        cur,
                        Tocino3DTorusTopology::Z_POS,
                        netDevices[x][y][ Inc(z) ],
                        Tocino3DTorusTopology::Z_NEG );

                TocinoChannelHelper(
                        cur,
                        Tocino3DTorusTopology::Z_NEG,
                        netDevices[x][y][ Dec(z) ],
                        Tocino3DTorusTopology::Z_POS );

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

uint32_t
Tocino3DTorusTopologyHelper::Middle() const
{
    // N.B. Truncation here means we return
    // the "lower middle" for even radix.
    return ( RADIX-1 ) / 2;
}

bool 
Tocino3DTorusTopologyHelper::CrossesBisection( Ptr<TocinoChannel> chan ) const
{
    uint32_t txCoord =
        chan->GetTocinoDevice( TocinoChannel::TX_DEV )->GetTocinoAddress().GetX();

    uint32_t rxCoord = 
        chan->GetTocinoDevice( TocinoChannel::RX_DEV )->GetTocinoAddress().GetX();

    // include links the cross the "middle"
    if( ( txCoord == Middle() ) && ( rxCoord == Middle()+1 ) )
    {
        return true;
    }
    
    if( ( txCoord == Middle()+1 ) && ( rxCoord == Middle() ) )
    {
        return true;
    }
   
    // include wrap-around links
    if( ( txCoord == RADIX-1 ) && ( rxCoord == 0 ) )
    {
        return true;
    }
    
    if( ( txCoord == 0 ) && ( rxCoord == RADIX-1 ) )
    {
        return true;
    }

    return false;
}

void 
Tocino3DTorusTopologyHelper::ReportBisectionBandwidth(
        const Tocino3DTorusNetDeviceContainer& netDevices,
        const Time duration ) const
{
    // N.B.  We require an even radix in order to bisect
    // the volume into equally-sized halves.
    NS_ASSERT( ( RADIX & 1 ) == 0 );
   
    // ISSUE-REVIEW:
    // Eventually we should support different radices per
    // dimension.  At that point, it may be beneficial to 
    // support selection of which dimension to bisect.
    
    uint32_t bisectionBytes = 0;
   
    for( uint32_t x = 0; x < RADIX; x++ )
    { 
        for( uint32_t y = 0; y < RADIX; y++ )
        { 
            for( uint32_t z = 0; z < RADIX; z++ )
            {
                Ptr<TocinoNetDevice> tnd = netDevices[x][y][z];
                
                for( uint32_t port = 0; port < tnd->GetNPorts()-1; ++port )
                {
                    Ptr<TocinoChannel> chan = tnd->GetChannel( port );

                    if( CrossesBisection( chan ) )
                    {
                        bisectionBytes += chan->GetTotalBytesTransmitted();
                    }
                }

            }
        }
    }

    //NS_LOG_UNCOND( "Total bytes crossing bisection: " << bisectionBytes );

    double bps = static_cast<double>(bisectionBytes) * 8 / duration.GetSeconds();

    std::cout << "Bisection bandwidth: " << bps/1024 << " Kbps" << std::endl;
    std::cout << "Bisection bandwidth: " << bps/1024/1024 << " Mbps" << std::endl;
    std::cout << "Bisection bandwidth: " << bps/1024/1024/1024 << " Gbps" << std::endl;
}

}
