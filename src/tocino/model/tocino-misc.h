/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_MISC_H__
#define __TOCINO_MISC_H__

#include <stdint.h>
#include <limits>

#include "tocino-type-safe-uint32.h"

#define STATIC_ASSERT( condition, name )\
    typedef char assert_failed_ ## name [ (condition) ? 1 : -1 ];

namespace ns3
{
    const uint32_t TOCINO_INVALID_PORT = std::numeric_limits<uint32_t>::max();
    const uint8_t TOCINO_INVALID_VC = std::numeric_limits<uint8_t>::max();

    // We must ensure log2(MAX_VCS) == VC_BITS
    const uint32_t TOCINO_NUM_VC_BITS = 4;
    const uint32_t TOCINO_MAX_VCS = 16;

    void TocinoCustomizeLogging();

    DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoInputVC );
    DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoOutputVC );

    DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoInputPort );
    DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoOutputPort );
}

#endif //__TOCINO_MISC_H__
