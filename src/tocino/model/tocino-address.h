/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ADDRESS_H__
#define __TOCINO_ADDRESS_H__

#include <stdint.h>

#include "ns3/mac48-address.h"

namespace ns3 {

class TocinoAddress
{
    public:

    TocinoAddress()
    {
        m_address.raw = 0;
    }

    TocinoAddress( uint32_t a )
    {
        m_address.raw = a;
    }
    
    TocinoAddress( uint8_t x, uint8_t y, uint8_t z, uint8_t res = 0 )
    {
        m_address.x = x;
        m_address.y = y;
        m_address.z = z;
        m_address.reserved = res;
    }

    Address ConvertTo() const
    {
        return Address( GetType(),
            reinterpret_cast<const uint8_t*>( &m_address.raw ),
            sizeof( m_address ) );
    }

    operator Address() const
    {
        return ConvertTo();
    }

    static TocinoAddress ConvertFrom( const Address &a )
    {
        TocinoAddress ta;
        NS_ASSERT( a.CheckCompatible( GetType(), sizeof( m_address ) ) );
        a.CopyTo( reinterpret_cast<uint8_t*>( &ta.m_address.raw ) );
        return ta;
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
    
    uint8_t GetX()
    {
        return m_address.x;
    }
    
    uint8_t GetY()
    {
        return m_address.y;
    }
    
    uint8_t GetZ()
    {
        return m_address.z;
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
            uint8_t x, y, z;
            uint8_t reserved  : 7;
            uint8_t multicast : 1;
        };
        uint32_t raw;
    }
    m_address;
};

}

#endif /* __TOCINO_ADDRESS_H__ */

