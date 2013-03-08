/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ADDRESS_H__
#define __TOCINO_ADDRESS_H__

#include <stdint.h>

#include "ns3/mac48-address.h"

#include "tocino-misc.h"

namespace ns3 {

class TocinoAddress
{
    public:

    TocinoAddress()
    {
        m_address.raw = 0;
        
        m_isValid = false;
    }

    explicit TocinoAddress( uint32_t a )
    {
        m_address.raw = a;
        
        m_isValid = true;
    }
    
    typedef uint8_t Coordinate;
    
    TocinoAddress( Coordinate x, Coordinate y, Coordinate z, uint8_t res = 0 )
    {
        m_address.x = x;
        m_address.y = y;
        m_address.z = z;
        m_address.reserved = res;
        m_address.multicast = 0;
        
        m_isValid = true;
    }

    Address ConvertTo() const
    {
        return Address( GetType(),
            reinterpret_cast<const uint8_t*>( &m_address.raw ),
            GetLength() );
    }

    operator Address() const
    {
        return ConvertTo();
    }

    static TocinoAddress ConvertFrom( const Address &a )
    {
        TocinoAddress ta;
        NS_ASSERT( a.CheckCompatible( GetType(), GetLength() ) );
        a.CopyTo( reinterpret_cast<uint8_t*>( &ta.m_address.raw ) );
        ta.m_isValid = !a.IsInvalid();
        return ta;
    }
    
    bool IsValid() const
    {
        return m_isValid;
    }
    
    bool IsInvalid() const
    {
        return !m_isValid;
    }

    bool IsMulticast()
    {
        return m_address.multicast;
    }

    void SetMulticast()
    {
        m_address.multicast = true; 
    }
    
    Mac48Address AsMac48Address() const
    {
        uint8_t buf[6];
       
        // Microsoft XCG OUI
        buf[5] = 0xDC;
        buf[4] = 0xB4;
        buf[3] = 0xC4;

        // embed XYZ in lower 3B
        buf[2] = m_address.x;
        buf[1] = m_address.y;
        buf[0] = m_address.z;

        Mac48Address a;
        a.CopyFrom( buf );

        return a;
    }
    
    Coordinate GetX() const
    {
        return m_address.x;
    }
    
    Coordinate GetY() const
    {
        return m_address.y;
    }
    
    Coordinate GetZ() const
    {
        return m_address.z;
    }
  
    static const int MAX_DIM = 3;

    Coordinate GetCoordinate( TocinoDimension d ) const
    {
        NS_ASSERT( d != TOCINO_INVALID_DIMENSION );
        NS_ASSERT( d < MAX_DIM );

        return m_address.coord[ d.AsUInt32() ];
    }

    static uint8_t GetLength()
    {
        return sizeof( m_address );
    }

    private:

    static uint8_t GetType()
    {
        static uint8_t type = Address::Register();
        return type;
    }

    union
    {
        struct
        {
            union
            {
                struct
                {
                    Coordinate x, y, z;
                };
                Coordinate coord[MAX_DIM];
            };

            uint8_t reserved  : 7;
            uint8_t multicast : 1;
        };
        uint32_t raw;
    }
    m_address;

    bool m_isValid;
};

}

#endif /* __TOCINO_ADDRESS_H__ */

