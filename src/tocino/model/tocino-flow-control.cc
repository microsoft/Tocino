/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/packet.h"

#include "tocino-flow-control.h"
#include "tocino-flit-header.h"

namespace ns3
{

Ptr< Packet>
TocinoFlowControl::GetLLCPacket( const VCBitSet& xon, const VCBitSet& xoff )
{
    Payload data;

    data.xonMask = xon.to_ulong();
    data.xoffMask = xoff.to_ulong();

    Ptr<Packet> p =
        Create<Packet>( reinterpret_cast<uint8_t*>( &data ), sizeof(data) );

    TocinoFlitHeader h; // src, dest?
    h.SetHead();
    h.SetTail();
    h.SetType( TocinoFlitHeader::LLC );

    p->AddHeader(h);

    return p;
}

// FIXME: This interface is deprecated and should be removed
Ptr< Packet>
TocinoFlowControl::GetXONPacket()
{
    return GetLLCPacket( -1, 0 );
}

// FIXME: This interface is deprecated and should be removed
Ptr< Packet>
TocinoFlowControl::GetXOFFPacket()
{
    return GetLLCPacket( 0, -1 );
}

bool
TocinoFlowControl::IsLLCPacket( Ptr<const Packet> pkt )
{
    // Make a copy so we can safely RemoveHeader() later
    Ptr<Packet> p = pkt->Copy();

    // We *must* remove, not merely peek, otherwise 
    // subsequent CopyData() will return header & payload
    TocinoFlitHeader h;
    p->RemoveHeader(h);

    if( h.GetType() == TocinoFlitHeader::LLC )
    {
        return true;
    }

    return false;
}

VCBitSet
TocinoFlowControl::GetXONBits( Ptr<const Packet> pkt )
{
    // Make a copy so we can safely RemoveHeader() later
    Ptr<Packet> p = pkt->Copy();

    // We *must* remove, not merely peek, otherwise 
    // subsequent CopyData() will return header & payload
    TocinoFlitHeader h;
    p->RemoveHeader(h);

    NS_ASSERT( h.GetType() == TocinoFlitHeader::LLC );
    NS_ASSERT( h.IsHead() );
    NS_ASSERT( h.IsTail() );

    Payload data;

    p->CopyData( reinterpret_cast<uint8_t*>( &data ), sizeof(data) );

    return data.xonMask;
}

VCBitSet
TocinoFlowControl::GetXOFFBits( Ptr<const Packet> pkt )
{
    // Make a copy so we can safely RemoveHeader() later
    Ptr<Packet> p = pkt->Copy();

    // We *must* remove, not merely peek, otherwise 
    // subsequent CopyData() will return header & payload
    TocinoFlitHeader h;
    p->RemoveHeader(h);

    NS_ASSERT( h.GetType() == TocinoFlitHeader::LLC );
    NS_ASSERT( h.IsHead() );
    NS_ASSERT( h.IsTail() );

    Payload data;

    p->CopyData( reinterpret_cast<uint8_t*>( &data ), sizeof(data) );

    return data.xoffMask;
}

// FIXME: This interface is deprecated and should be removed
bool
TocinoFlowControl::IsXONPacket( Ptr<const Packet> pkt )
{
    if( !IsLLCPacket( pkt ) )
    {
        return false;
    }

    return GetXONBits( pkt ).any();
}

// FIXME: This interface is deprecated and should be removed
bool
TocinoFlowControl::IsXOFFPacket( Ptr<const Packet> pkt )
{
    if( !IsLLCPacket( pkt ) )
    {
        return false;
    }

    return GetXOFFBits( pkt ).any();
}
}
