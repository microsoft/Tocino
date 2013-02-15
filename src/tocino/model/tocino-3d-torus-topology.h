/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_3D_TORUS_TOPOLOGY_H__
#define __TOCINO_3D_TORUS_TOPOLOGY_H__

#include <string>
#include <stdint.h>

#include "tocino-misc.h"

namespace ns3
{

class Tocino3DTorusTopology
{
    public:

    enum Dimension
    {
        X = 0,
        Y,
        Z
    };

    enum Port
    {
        X_POS = 0,
        X_NEG,
        Y_POS,
        Y_NEG,
        Z_POS,
        Z_NEG,
        HOST
    };

    static std::string PortNumberToString( const uint32_t );
    static std::string PortNumberToString( const TocinoInputPort );
    static std::string PortNumberToString( const TocinoOutputPort );
};

}

#endif // __TOCINO_3D_TORUS_TOPOLOGY_H__
