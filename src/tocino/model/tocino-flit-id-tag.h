/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_FLIT_ID_TAG_H__
#define __TOCINO_FLIT_ID_TAG_H__

#include <string>

#include "ns3/packet.h"
#include "ns3/tag.h"

namespace ns3 {

class TocinoFlitIdTag : public Tag
{
    public:
    static TypeId GetTypeId();

    virtual TypeId GetInstanceTypeId() const;

    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize( TagBuffer buf ) const;
    virtual void Deserialize( TagBuffer buf );

    virtual void Print( std::ostream &os ) const;

    TocinoFlitIdTag();
    TocinoFlitIdTag( uint32_t, uint32_t, uint32_t );

    static uint32_t NextPacketNumber();

    private:

    // ISSUE-REVIEW: might want uint64_t here?
    uint32_t m_absolutePacketNumber;

    // ISSUE-REVIEW: these could probably be uint16_4
    uint32_t m_relativeFlitNumber;
    uint32_t m_totalFlitsInPacket;
};
    
std::string GetTocinoFlitIdString( Ptr<const Packet> );

} // namespace ns3

#endif // __TOCINO_FLIT_ID_TAG_H__
