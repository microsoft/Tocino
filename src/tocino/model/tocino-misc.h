/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_MISC_H__
#define __TOCINO_MISC_H__

#include <stdint.h>
#include <limits>
#include <string>

#define STATIC_ASSERT( condition, name )\
    typedef char assert_failed_ ## name [ (condition) ? 1 : -1 ];

namespace ns3
{
    const uint32_t TOCINO_INVALID_PORT = std::numeric_limits<uint32_t>::max();
    const uint8_t TOCINO_INVALID_VC = std::numeric_limits<uint8_t>::max();

    // ISSUE-REVIEW: consider moving this to its own file
    struct TocinoQueueDescriptor
    {
        uint32_t port;
        uint8_t vc;

        TocinoQueueDescriptor( uint32_t port_, uint8_t vc_ )
            : port( port_ )
            , vc( vc_ )
        {}
        
        bool operator==( const TocinoQueueDescriptor& other )
        {
            if( other.port != port )
                return false;

            if( other.vc != vc )
                return false;

            return true;
        }
        
        bool operator!=( const TocinoQueueDescriptor& other )
        {
            return !( *this == other );
        }
    };
    
    static const TocinoQueueDescriptor TOCINO_INVALID_QUEUE
        = TocinoQueueDescriptor( TOCINO_INVALID_PORT, TOCINO_INVALID_VC );

    // We must ensure log2(MAX_VCS) == VC_BITS
    const uint32_t TOCINO_NUM_VC_BITS = 4;
    const uint32_t TOCINO_MAX_VCS = 16;

    void TocinoCustomizeLogging();

    std::string Tocino3dTorusPortNumberToString( const int port );
}

#endif //__TOCINO_MISC_H__
