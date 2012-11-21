/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_FLOW_CONTROL_H__
#define __TOCINO_FLOW_CONTROL_H__

#include <bitset>

#include "ns3/ptr.h"

#include "tocino-misc.h"

namespace ns3
{

class Packet;

typedef std::bitset<TOCINO_NUM_VC_BITS> VCBitSet;

class TocinoFlowControl
{
    public:

    enum State {XOFF, XON};
    
    static Ptr<Packet> GetLLCPacket( const VCBitSet& xon, const VCBitSet& xoff );
    
    static bool IsLLCPacket( Ptr<const Packet> );

    static VCBitSet GetXONBits( Ptr<const Packet> );
    static VCBitSet GetXOFFBits( Ptr<const Packet> );
   
    // deprecated
    static Ptr<Packet> GetXONPacket();
    static Ptr<Packet> GetXOFFPacket();
    static bool IsXONPacket( Ptr<const Packet> );
    static bool IsXOFFPacket( Ptr<const Packet> );

    private:
    
    struct Payload
    {
        unsigned long xonMask;
        unsigned long xoffMask;
    };

    template< State TFCS >
        static Ptr<Packet> GetPacketHelper( const VCBitSet& );

    template< State TFCS >
        static bool TestPacketHelper( Ptr<Packet> p );
};

}

#endif // __TOCINO_FLOW_CONTROL_H__
