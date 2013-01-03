#include "ns3/address-utils.h"

#include "tocino-flit-header.h"
#include "tocino-address.h"
#include "tocino-misc.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED( TocinoFlitHeader );

const unsigned TocinoFlitHeader::FLIT_LENGTH = 64;
const unsigned TocinoFlitHeader::MAX_PAYLOAD_HEAD = 40;
const unsigned TocinoFlitHeader::MAX_PAYLOAD_OTHER = 62;
const unsigned TocinoFlitHeader::SIZE_HEAD = FLIT_LENGTH - MAX_PAYLOAD_HEAD;
const unsigned TocinoFlitHeader::SIZE_OTHER = FLIT_LENGTH - MAX_PAYLOAD_OTHER;

TypeId TocinoFlitHeader::GetTypeId( void )
{
  static TypeId tid = TypeId( "ns3::TocinoFlitHeader" )
    .SetParent<Header>()
    .AddConstructor<TocinoFlitHeader>();

  return tid;
}

TocinoFlitHeader::TocinoFlitHeader()
    : m_src( 0 )
    , m_dst( 0 )
    , m_isHead( false )
    , m_isTail( false )
    , m_virtualChannel( 0 )
    , m_length( 0 )
    , m_type( INVALID )
{}


TocinoFlitHeader::TocinoFlitHeader( TocinoAddress src, TocinoAddress dst )
    : m_src( src )
    , m_dst( dst )
    , m_isHead( false )
    , m_isTail( false )
    , m_virtualChannel( 0 )
    , m_length( 0 )
    , m_type( INVALID )
{}

TypeId TocinoFlitHeader::GetInstanceTypeId( void ) const
{
    return GetTypeId();
}

void TocinoFlitHeader::Print( std::ostream &os ) const
{
    NS_ASSERT_MSG( false, "Not yet implemented" );
}

uint32_t TocinoFlitHeader::GetSerializedSize( void ) const
{
    if( m_isHead )
    {
        return SIZE_HEAD;
    }
    
    return SIZE_OTHER;
}

// If this fires, you may need to modify the union below
STATIC_ASSERT( TOCINO_NUM_VC_BITS == 4, flit_header_assumes_16_virtual_channels );

namespace {
    union Flags
    {
        struct
        {
            // lsB (lsb first) 
            unsigned vc         : 4;
            unsigned type       : 4;

            // msB (lsb first)
            unsigned length     : 6;
            unsigned tail       : 1;
            unsigned head       : 1;
        };
        uint16_t asU16;
    };
}

void TocinoFlitHeader::Serialize( Buffer::Iterator i ) const
{
    Flags f;
    
    f.head = m_isHead;
    f.tail = m_isTail;
    f.length = m_length;
    f.type = m_type;
    f.vc = m_virtualChannel;

    i.WriteU16( f.asU16 );
        
    if( m_isHead )
    {
        // TODO: set this to something real
        uint8_t seqnum[6] = {0};
        i.Write( seqnum, sizeof(seqnum) );

        // TODO: set this to something real
        uint8_t timestamp[8] = {0};
        i.Write( timestamp, sizeof(timestamp) );
  
        // TODO ensure these are each 4B
        WriteTo( i, m_src );
        WriteTo( i, m_dst );
    }
}

uint32_t TocinoFlitHeader::Deserialize( Buffer::Iterator i )
{
    Flags f;

    f.asU16 = i.ReadU16();

    m_isHead = f.head;
    m_isTail = f.tail;
    m_length = f.length;
    m_type = CheckedConvertToType( f.type );
    m_virtualChannel = f.vc;
        
    if( m_isHead )
    {
        // FIXME: where does this go? 
        uint8_t seqnum[6] = {0};
        i.Read( seqnum, sizeof(seqnum) );

        // FIXME: where does this go? 
        uint8_t timestamp[8] = {0};
        i.Read( timestamp, sizeof(timestamp) );
  
        Address a;

        ReadFrom( i, a, sizeof( TocinoAddress ) );
        m_src = TocinoAddress::ConvertFrom( a );

        ReadFrom( i, a, sizeof( TocinoAddress ) );
        m_dst = TocinoAddress::ConvertFrom( a );
    }

    return GetSerializedSize(); 
}

void TocinoFlitHeader::SetSource( TocinoAddress src )
{
    NS_ASSERT( m_isHead );
    m_src = src;
}

void TocinoFlitHeader::SetDestination( TocinoAddress dest )
{
    NS_ASSERT( m_isHead );
    m_dst = dest;
}

TocinoAddress TocinoFlitHeader::GetSource()
{
    NS_ASSERT( m_isHead );
    return m_src;
}

TocinoAddress TocinoFlitHeader::GetDestination()
{
    NS_ASSERT( m_isHead );
    return m_dst;
}
    
bool TocinoFlitHeader::IsHead() const
{
    return m_isHead;
}

bool TocinoFlitHeader::IsTail() const
{
    return m_isTail;
}

void TocinoFlitHeader::SetHead()
{
    m_isHead = true;
}

void TocinoFlitHeader::SetTail()
{
    m_isTail = true;
}

void TocinoFlitHeader::ClearHead()
{
    m_isHead = false;
}

void TocinoFlitHeader::ClearTail()
{
    m_isTail = false;
}

void TocinoFlitHeader::SetVirtualChannel( uint8_t vc )
{
    m_virtualChannel = vc;
}

uint8_t TocinoFlitHeader::GetVirtualChannel()
{
    return m_virtualChannel;
}
    
void TocinoFlitHeader::SetLength( uint8_t l )
{
    m_length = l;
}

uint8_t TocinoFlitHeader::GetLength()
{
    return m_length;
}

void TocinoFlitHeader::SetType( Type t )
{
    NS_ASSERT_MSG( ( t >= MIN_TYPE ) && ( t <= MAX_TYPE ), "Invalid type" );
    m_type = t;
}

TocinoFlitHeader::Type TocinoFlitHeader::GetType()
{
    return m_type;
}

TocinoFlitHeader::Type TocinoFlitHeader::CheckedConvertToType( int t )
{
    NS_ASSERT_MSG( ( t >= MIN_TYPE ) && ( t <= MAX_TYPE ),
        "Integer out of range while converting to TocinoFlitHeader::Type" );

    return static_cast<TocinoFlitHeader::Type>( t );
}

}
