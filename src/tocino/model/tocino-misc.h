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
        // N.B.
        //
        // Whether this represents input port or output port
        // is in "the eye of the client."  In most cases it
        // will be an output port.  Arbiter, however, considers
        // it an input port.
        //
        // We call the member simply "port" to avoid confusion.
        uint32_t port;

        uint8_t inputVC;
        uint8_t outputVC;
        
        TocinoQueueDescriptor( uint32_t p, uint8_t inVC, uint8_t outVC )
            : port( p )
            , inputVC( inVC )
            , outputVC( outVC )
        {}
        
        bool operator==( const TocinoQueueDescriptor& other ) const
        {
            if( other.port != port )
                return false;

            if( other.inputVC != inputVC )
                return false;
            
            if( other.outputVC != outputVC )
                return false;

            return true;
        }
        
        bool operator!=( const TocinoQueueDescriptor& other ) const
        {
            return !( *this == other );
        }
    };
    
    static const TocinoQueueDescriptor
        TOCINO_INVALID_QUEUE( TOCINO_INVALID_PORT, 
                TOCINO_INVALID_VC, TOCINO_INVALID_VC );

    // We must ensure log2(MAX_VCS) == VC_BITS
    const uint32_t TOCINO_NUM_VC_BITS = 4;
    const uint32_t TOCINO_MAX_VCS = 16;

    void TocinoCustomizeLogging();

    std::string Tocino3dTorusPortNumberToString( const int port );
}

#endif //__TOCINO_MISC_H__
