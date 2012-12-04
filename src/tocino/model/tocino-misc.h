/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_MISC_H__
#define __TOCINO_MISC_H__

#include <stdint.h>
#include <limits>

#define STATIC_ASSERT( condition, name )\
    typedef char assert_failed_ ## name [ (condition) ? 1 : -1 ];

namespace ns3
{
    const uint32_t TOCINO_INVALID_PORT = std::numeric_limits<uint32_t>::max();
    const uint32_t TOCINO_INVALID_QUEUE = std::numeric_limits<uint32_t>::max();
    const uint8_t TOCINO_INVALID_VC = std::numeric_limits<uint8_t>::max();
   
    // We must ensure log2(MAX_VCS) == VC_BITS
    const uint32_t TOCINO_NUM_VC_BITS = 4;
    const uint32_t TOCINO_MAX_VCS = 16;

    void TocinoCustomizeLogging();
}

#endif //__TOCINO_MISC_H__
