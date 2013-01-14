/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_QUEUE_DESCRIPTOR_H__
#define __TOCINO_QUEUE_DESCRIPTOR_H__

#include "tocino-misc.h"

namespace ns3
{
    // ISSUE-REVIEW
    // Many if not all of the uses of this gizmo could
    // be replaced with a std::tuple and std::tie.
    class TocinoQueueDescriptor
    {
        public:
        
        TocinoQueueDescriptor( uint32_t port, uint32_t vc )
            : m_port( port )
            , m_vc( vc )
        {}
        
        bool operator==( const TocinoQueueDescriptor& other ) const
        {
            if( other.m_port != m_port )
                return false;

            if( other.m_vc != m_vc )
                return false;

            return true;
        }
        
        bool operator!=( const TocinoQueueDescriptor& other ) const
        {
            return !( *this == other );
        }

        uint32_t GetPort() const
        {
            return m_port;
        }
        
        uint32_t GetVirtualChannel() const
        {
            return m_vc;
        }
    
        private:

        uint32_t m_port;
        uint32_t m_vc;
    };

    static const TocinoQueueDescriptor
        TOCINO_INVALID_QUEUE( TOCINO_INVALID_PORT, TOCINO_INVALID_VC );
}

#endif
