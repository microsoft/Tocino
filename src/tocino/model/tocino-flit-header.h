#ifndef __TOCINO_FLIT_HEADER_H__
#define __TOCINO_FLIT_HEADER_H__

#include "ns3/header.h"

namespace ns3 {

class TocinoFlitHeader : public Header
{
    public:

    TocinoFlitHeader();
    
    static TypeId GetTypeId( void );

    virtual TypeId GetInstanceTypeId( void ) const;
    virtual void Print( std::ostream &os ) const;
    virtual uint32_t GetSerializedSize( void ) const;
    virtual void Serialize( Buffer::Iterator start ) const;
    virtual uint32_t Deserialize( Buffer::Iterator start );

    private:
};

}

#endif // __TOCINO_FLIT_HEADER_H
