#ifndef __TOCINO_FLIT_HEADER_H__
#define __TOCINO_FLIT_HEADER_H__

#include "ns3/header.h"

#include "tocino-address.h"

namespace ns3 {

class TocinoFlitHeader : public Header
{
    public:
    
    static TypeId GetTypeId( void );
    
    TocinoFlitHeader();
    TocinoFlitHeader( TocinoAddress src, TocinoAddress dst );

    virtual TypeId GetInstanceTypeId( void ) const;
    virtual void Print( std::ostream &os ) const;
    virtual uint32_t GetSerializedSize( void ) const;
    virtual void Serialize( Buffer::Iterator start ) const;
    virtual uint32_t Deserialize( Buffer::Iterator start );

    void SetSource( TocinoAddress );
    void SetDestination( TocinoAddress );
    
    TocinoAddress GetSource();
    TocinoAddress GetDestination();

    bool IsHead() const;
    bool IsTail() const;
    
    void SetHead();
    void SetTail();
    
    void ClearHead();
    void ClearTail();

    void SetVirtualChannel( uint8_t );
    uint8_t GetVirtualChannel();

    static const unsigned FLIT_LENGTH = 64;
    static const unsigned MAX_PAYLOAD_HEAD = 40;
    static const unsigned MAX_PAYLOAD_OTHER = 62;
    static const unsigned SIZE_HEAD = FLIT_LENGTH - MAX_PAYLOAD_HEAD;
    static const unsigned SIZE_OTHER = FLIT_LENGTH - MAX_PAYLOAD_OTHER;

    void SetLength( uint8_t );
    uint8_t GetLength();
   
    enum Type
    {   
        INVALID,
        LLC, 
        ETHERNET,
        SATA,
        IPV4,
        IPV6,
        MIN_TYPE = INVALID,
        MAX_TYPE = IPV6
    };

    void SetType( Type );
    uint8_t GetType();
    
    static Type CheckedConvertToType( int );

    private:
   
    TocinoAddress m_src;
    TocinoAddress m_dst;

    bool m_isHead;
    bool m_isTail;
    
    uint8_t m_virtualChannel;
    
    uint8_t m_length;

    Type m_type;
};

}

#endif // __TOCINO_FLIT_HEADER_H
