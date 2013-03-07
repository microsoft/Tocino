#include "ns3/simulator.h"
#include "ns3/simulator-impl.h"
#include "ns3/log.h"

#include "tocino-misc.h"
#include "tocino-flit-header.h"
#include "tocino-flit-id-tag.h"

namespace
{

void TocinoTimePrinter( std::ostream &os )
{
    // Coerce logging to display nanoseconds
    os << ns3::Simulator::Now().GetNanoSeconds() << "ns";
}

}

namespace ns3
{

void TocinoCustomizeLogging()
{
    // HACK: Make the Simulator initialize itself
    // so we can override its preferred time printer
    Ptr<SimulatorImpl> hack = Simulator::GetImplementation();
    LogSetTimePrinter( &TocinoTimePrinter );
}

TocinoDirection
TocinoGetDirection( const TocinoPort port )
{
    if( (port == TOCINO_PORT_X_POS) ||
        (port == TOCINO_PORT_Y_POS) ||
        (port == TOCINO_PORT_Z_POS) )
    {
        return TOCINO_DIRECTION_POS;
    }
    else if( (port == TOCINO_PORT_X_NEG) ||
             (port == TOCINO_PORT_Y_NEG) ||
             (port == TOCINO_PORT_Z_NEG) )
    {
        return TOCINO_DIRECTION_NEG;
    }
    
    return TOCINO_INVALID_DIRECTION;
}

TocinoDimension
TocinoGetDimension( const TocinoPort port )
{
    if( (port == TOCINO_PORT_X_POS) ||
        (port == TOCINO_PORT_X_NEG) )
    {
        return TOCINO_DIMENSION_X;
    }
    else if( (port == TOCINO_PORT_Y_POS) ||
             (port == TOCINO_PORT_Y_NEG) )
    {
        return TOCINO_DIMENSION_Y;
    }
    else if( (port == TOCINO_PORT_Z_POS) ||
             (port == TOCINO_PORT_Z_NEG) )
    {
        return TOCINO_DIMENSION_Z;
    }
    
    return TOCINO_INVALID_DIMENSION;
}

TocinoPort
TocinoGetPort(
        const TocinoDimension dim,
        const TocinoDirection dir )
{
    if( (dim == TOCINO_DIMENSION_X) && (dir == TOCINO_DIRECTION_POS) )
    {
        return TOCINO_PORT_X_POS;
    }
    else if( (dim == TOCINO_DIMENSION_X) && (dir == TOCINO_DIRECTION_NEG) )
    {
        return TOCINO_PORT_X_NEG;
    }
    else if( (dim == TOCINO_DIMENSION_Y) && (dir == TOCINO_DIRECTION_POS) )
    {
        return TOCINO_PORT_Y_POS;
    }
    else if( (dim == TOCINO_DIMENSION_Y) && (dir == TOCINO_DIRECTION_NEG) )
    {
        return TOCINO_PORT_Y_NEG;
    }
    else if( (dim == TOCINO_DIMENSION_Z) && (dir == TOCINO_DIRECTION_POS) )
    {
        return TOCINO_PORT_Z_POS;
    }
    else if( (dim == TOCINO_DIMENSION_Z) && (dir == TOCINO_DIRECTION_NEG) )
    {
        return TOCINO_PORT_Z_NEG;
    }

    return TOCINO_INVALID_PORT;
}

TocinoDirection
TocinoGetOppositeDirection( const TocinoDirection dir )
{
    NS_ASSERT( dir != TOCINO_INVALID_DIRECTION );

    if( dir == TOCINO_DIRECTION_POS )
    {
        return TOCINO_DIRECTION_NEG;
    }
    else
    {
        NS_ASSERT( dir == TOCINO_DIRECTION_NEG );
        return TOCINO_DIRECTION_POS;
    }

    return TOCINO_INVALID_DIRECTION;
}

std::string
TocinoDirectionToString(
        const TocinoDirection dir )
{
    std::ostringstream oss;

    if( dir == TOCINO_DIRECTION_POS )
    {
        oss << "+";
    }
    else if( dir == TOCINO_DIRECTION_NEG )
    {
        oss << "-";
    }

    return oss.str();
}

std::string
TocinoDimensionToString(
        const TocinoDimension dim )
{
    std::ostringstream oss;
    
    if( dim == TOCINO_DIMENSION_X )
    {
        oss << "X";
    }
    else if( dim == TOCINO_DIMENSION_Y )
    {
        oss << "Y";
    }
    else if( dim == TOCINO_DIMENSION_Z )
    {
        oss << "Z";
    }

    return oss.str();
}

std::string
TocinoPortToString(
        const TocinoPort port )
{
    if( port == TOCINO_PORT_HOST )
    {
        return "host";
    }

    std::ostringstream oss;

    TocinoDimension dim = TocinoGetDimension( port );
    TocinoDirection dir = TocinoGetDirection( port );

    oss << TocinoDimensionToString(dim)
        << TocinoDirectionToString(dir);

    return oss.str();
}

void
TocinoAddIntermediateDestination(
        TocinoFlittizedPacket& packet,
        const TocinoAddress& intermediateDest )
{
    // Given flittized packet going from A to C, 
    // introduce an intermediate destination B, such that
    // the packet will be routed from A, to B, and then to C.
    
    // We cloak the inner head flit as a regular 'ole body flit.
    
    // We then create a new "runt" head flit with the intermediate
    // destination and staple it to the front of the packet.
    
    TocinoFlitHeader innerHeader;
    Ptr<Packet> innerHeadFlit = packet.front();

    innerHeadFlit->RemoveHeader( innerHeader );
    
    TocinoFlitHeader outerHeader( innerHeader.GetSource(), intermediateDest );

    NS_ASSERT( innerHeader.IsHead() );
    innerHeader.CloakHead();

    innerHeadFlit->AddHeader( innerHeader );
    
    Ptr<Packet> outerHeadFlit = Create<Packet>();

    outerHeader.SetHead();
    outerHeader.SetType( TocinoFlitHeader::ENCAPSULATED_PACKET );
    outerHeader.SetLength( TocinoFlitHeader::SIZE_HEAD );
    outerHeader.SetVirtualChannel( innerHeader.GetVirtualChannel() );

    outerHeadFlit->AddHeader( outerHeader );

    packet.push_front( outerHeadFlit );
   
    // ISSUE-REVIEW: do we want to do something special with the
    // packet tags in the case of encapsulation?  Right now I am
    // just renumbering them.
    
    const uint32_t TOTAL_FLITS = packet.size();
    const uint32_t ABS_PACKET_NUM = GetTocinoAbsolutePacketNumber( packet.back() );

    for( uint32_t i = 0; i < TOTAL_FLITS; ++i )
    {
        const uint32_t REL_FLIT_NUM = i+1;
        
        TocinoFlitIdTag oldTag;
        packet[i]->RemovePacketTag( oldTag );

        TocinoFlitIdTag newTag( ABS_PACKET_NUM, REL_FLIT_NUM, TOTAL_FLITS );
        packet[i]->AddPacketTag( newTag );
    }
}

}
