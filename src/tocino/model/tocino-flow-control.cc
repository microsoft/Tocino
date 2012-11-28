/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/packet.h"

#include "tocino-flow-control.h"
#include "tocino-flit-header.h"

namespace ns3
{

Ptr<Packet>
GetTocinoFlowControlFlit( const TocinoFlowControlState& s )
{
    unsigned long data = s.to_ulong();

    Ptr<Packet> f =
        Create<Packet>( reinterpret_cast<uint8_t*>( &data ), sizeof(data) );

    TocinoFlitHeader h;
    h.SetHead();
    h.SetTail();
    h.SetType( TocinoFlitHeader::LLC );

    f->AddHeader(h);

    return f;
}

bool
IsTocinoFlowControlFlit( Ptr<const Packet> flit )
{
    // Make a copy so we can safely RemoveHeader() later
    Ptr<Packet> f = flit->Copy();

    // We *must* remove, not merely peek, otherwise 
    // subsequent CopyData() will return header & payload
    TocinoFlitHeader h;
    f->RemoveHeader(h);

    if( h.GetType() == TocinoFlitHeader::LLC )
    {
        NS_ASSERT( h.IsHead() );
        NS_ASSERT( h.IsTail() );
        return true;
    }

    return false;
}

TocinoFlowControlState
GetTocinoFlowControlState( Ptr<const Packet> flit )
{
    NS_ASSERT( IsTocinoFlowControlFlit( flit ) );

    // Make a copy so we can safely RemoveHeader() later
    Ptr<Packet> f = flit->Copy();

    // We *must* remove, not merely peek, otherwise 
    // subsequent CopyData() will return header & payload
    TocinoFlitHeader h;
    f->RemoveHeader(h);

    unsigned long data;

    f->CopyData( reinterpret_cast<uint8_t*>( &data ), sizeof(data) );

    return TocinoFlowControlState( data );
}

}
