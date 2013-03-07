/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include <sstream>

#include "tocino-flit-id-tag.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(TocinoFlitIdTag);

TypeId 
TocinoFlitIdTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TocinoFlitIdTag")
    .SetParent<Tag>()
    .AddConstructor<TocinoFlitIdTag>()
  ;
  return tid;
}

TypeId 
TocinoFlitIdTag::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t 
TocinoFlitIdTag::GetSerializedSize() const
{
  return 12;
}

void 
TocinoFlitIdTag::Serialize( TagBuffer buf ) const
{
  buf.WriteU32( m_absolutePacketNumber );
  buf.WriteU32( m_relativeFlitNumber );
  buf.WriteU32( m_totalFlitsInPacket );
}

void 
TocinoFlitIdTag::Deserialize( TagBuffer buf )
{
  m_absolutePacketNumber = buf.ReadU32();
  m_relativeFlitNumber = buf.ReadU32();
  m_totalFlitsInPacket = buf.ReadU32();
}

void 
TocinoFlitIdTag::Print( std::ostream &os ) const
{
  os << "fid="
     << m_absolutePacketNumber
     << "["
     << m_relativeFlitNumber
     << "/"
     << m_totalFlitsInPacket
     << "]";
}

TocinoFlitIdTag::TocinoFlitIdTag()
    : Tag() 
{}

TocinoFlitIdTag::TocinoFlitIdTag(
        uint32_t absolutePacketNumber,
        uint32_t relativeFlitNumber,
        uint32_t totalFlitsInPacket )
    : Tag() 
    , m_absolutePacketNumber( absolutePacketNumber )
    , m_relativeFlitNumber( relativeFlitNumber )
    , m_totalFlitsInPacket( totalFlitsInPacket )
{}

uint32_t
TocinoFlitIdTag::GetAbsolutePacketNumber() const
{
    return m_absolutePacketNumber;
}

uint32_t
TocinoFlitIdTag::NextPacketNumber()
{
    static uint32_t next = 0;
    return next++;
}

std::string
GetTocinoFlitIdString( Ptr<const Packet> flit )
{
    TocinoFlitIdTag tag;
    bool success = flit->PeekPacketTag( tag );
    NS_ASSERT( success );

    std::ostringstream oss;
    tag.Print( oss );

    return oss.str();
}

uint32_t
GetTocinoAbsolutePacketNumber( Ptr<const Packet> flit )
{
    TocinoFlitIdTag tag;
    bool success = flit->PeekPacketTag( tag );
    NS_ASSERT( success );

    return tag.GetAbsolutePacketNumber();
}

} // namespace ns3
