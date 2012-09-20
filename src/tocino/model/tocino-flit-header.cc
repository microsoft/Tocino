#include "tocino-flit-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED( TocinoFlitHeader );

TypeId TocinoFlitHeader::GetTypeId( void )
{
  static TypeId tid = TypeId( "ns3::TocinoFlitHeader" )
    .SetParent<Header>()
    .AddConstructor<TocinoFlitHeader>();

  return tid;
}

TocinoFlitHeader::TocinoFlitHeader()
{}

TypeId TocinoFlitHeader::GetInstanceTypeId( void ) const
{
    return GetTypeId();
}

void TocinoFlitHeader::Print( std::ostream &os ) const
{}

uint32_t TocinoFlitHeader::GetSerializedSize( void ) const
{
    return 0;
}

void TocinoFlitHeader::Serialize( Buffer::Iterator start ) const
{}

uint32_t TocinoFlitHeader::Deserialize( Buffer::Iterator start )
{
    return 0;
}

}
