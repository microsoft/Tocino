/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-3d-torus-topology.h"

#include <sstream>

#include "ns3/assert.h"

namespace ns3
{

std::string
Tocino3DTorusTopology::PortNumberToString(
        const uint32_t port )
{
    if( port == HOST )
    {
        return "host";
    }

    std::ostringstream oss;

    switch( port / 2 )
    {
        case X:
            oss << "x";
            break;

        case Y:
            oss << "y";
            break;

        case Z:
            oss << "z";
            break;

        default:
            NS_ASSERT( false );
            oss << "UNKNOWN PORT!";
            break;
    }

    if( port % 2 == 0 )
    {
        oss << "+";
    }
    else
    {
        oss << "-";
    }

    return oss.str();
}

std::string
Tocino3DTorusTopology::PortNumberToString(
        const TocinoInputPort inputPort )
{
    return PortNumberToString( inputPort.AsUInt32() ); 
}

std::string
Tocino3DTorusTopology::PortNumberToString(
        const TocinoOutputPort outputPort )
{
    return PortNumberToString( outputPort.AsUInt32() ); 
}

}
