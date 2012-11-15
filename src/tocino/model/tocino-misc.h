/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_MISC_H__
#define __TOCINO_MISC_H__

#include <stdint.h>
#include <limits>

namespace ns3
{
    const uint32_t TOCINO_INVALID_PORT = std::numeric_limits<uint32_t>::max();
    const uint32_t TOCINO_INVALID_QUEUE = std::numeric_limits<uint32_t>::max();

    void TocinoCustomizeLogging();
}

#endif //__TOCINO_MISC_H__
