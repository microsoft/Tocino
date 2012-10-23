/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/packet.h"

#include "tocino-flow-control.h"
#include "tocino-flit-header.h"

namespace ns3
{

template< TocinoFlowControl::State TFCS >
Ptr<const Packet> TocinoFlowControl::GetPacketHelper()
{
    static Ptr<Packet> p = NULL;

    if( p == NULL )
    {
        uint8_t DATA = TFCS;
        p = Create<Packet>( &DATA, sizeof(DATA) );

        TocinoFlitHeader h; // src, dest?
        h.SetHead();
        h.SetTail();
        h.SetType( TocinoFlitHeader::LLC );

        p->AddHeader(h);
    }

    return p;
}

template< TocinoFlowControl::State TFCS >
bool TocinoFlowControl::TestPacketHelper( Ptr<const Packet> p )
{
    TocinoFlitHeader h;
    p->PeekHeader(h);

    if( h.GetType() == TocinoFlitHeader::LLC )
    {
        NS_ASSERT( h.IsHead() );
        NS_ASSERT( h.IsTail() );

        uint8_t DATA;

        p->CopyData( &DATA, sizeof(DATA) );

        return ( DATA == TFCS );
    }

    return false;
}

Ptr<const Packet> TocinoFlowControl::GetXONPacket()
{
    return GetPacketHelper<XON>();
}

Ptr<const Packet> TocinoFlowControl::GetXOFFPacket()
{
    return GetPacketHelper<XOFF>();
}

bool TocinoFlowControl::IsXONPacket( Ptr<const Packet> p ) // const?
{
    return TestPacketHelper<XON>( p ); 
}

bool TocinoFlowControl::IsXOFFPacket( Ptr<const Packet> p ) // const?
{
    return TestPacketHelper<XOFF>( p ); 
}

}
