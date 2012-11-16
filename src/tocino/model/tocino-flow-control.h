/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_FLOW_CONTROL_H__
#define __TOCINO_FLOW_CONTROL_H__

#include "ns3/ptr.h"

namespace ns3
{

class Packet;

class TocinoFlowControl
{
    public:

    enum State {XOFF, XON};

    static Ptr<Packet> GetXONPacket( uint8_t vc = 0 );
    static Ptr<Packet> GetXOFFPacket( uint8_t vc = 0 );
    static bool IsXONPacket( Ptr<Packet> );
    static bool IsXOFFPacket( Ptr<Packet> );

    private:

    template< State TFCS >
        static Ptr<Packet> GetPacketHelper( uint8_t );

    template< State TFCS >
        static bool TestPacketHelper( Ptr<Packet> p );
};

}

#endif // __TOCINO_FLOW_CONTROL_H__
