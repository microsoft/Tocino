/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TEST_RESULTS_H__
#define __TOCINO_TEST_RESULTS_H__

#include <stdint.h>
#include <map>

#include "ns3/ptr.h"
#include "ns3/address.h"

#include "ns3/tocino-address.h"

namespace ns3
{

class NetDevice;
class Packet;

class TocinoTestResults
{
    public:

    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address& );

    void Reset();
   
    unsigned GetCount( TocinoAddress, TocinoAddress ) const;
    unsigned GetBytes( TocinoAddress, TocinoAddress ) const;

    unsigned GetTotalCount() const;
    unsigned GetTotalBytes() const;

    private:

    // 2d map src*dst => unsigned
    typedef std::map< TocinoAddress, unsigned > TestResultsRow;
    typedef std::map< TocinoAddress, TestResultsRow  > TestResults;

    TestResults m_counts;
    TestResults m_bytes;
};

}

#endif // __TOCINO_TEST_RESULTS_H__
