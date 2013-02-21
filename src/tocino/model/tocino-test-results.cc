/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-test-results.h"

#include "ns3/net-device.h"
#include "ns3/packet.h"

namespace ns3
{

bool
TocinoTestResults::AcceptPacket(
        Ptr<NetDevice> nd,
        Ptr<const Packet> p,
        uint16_t,
        const Address& src )
{
    TocinoAddress tsrc = TocinoAddress::ConvertFrom( src );
    TocinoAddress tdst = TocinoAddress::ConvertFrom( nd->GetAddress() );

    m_counts[tsrc][tdst]++;
    m_bytes[tsrc][tdst] += p->GetSize();

    return true;
}

void
TocinoTestResults::Reset()
{
    m_counts.clear();
    m_bytes.clear();
}

unsigned
TocinoTestResults::GetCount(
        TocinoAddress src,
        TocinoAddress dst ) const
{
    TestResults::const_iterator iter = m_counts.find(src);
   
    if( iter == m_counts.end() )
    {
        return 0;
    }

    const TestResultsRow& row = iter->second;

    TestResultsRow::const_iterator rowIter = row.find(dst);
    
    if( rowIter == row.end() )
    {
        return 0;
    }

    return rowIter->second;
}

unsigned
TocinoTestResults::GetBytes(
        TocinoAddress src,
        TocinoAddress dst ) const
{
    TestResults::const_iterator iter = m_bytes.find(src);
   
    if( iter == m_counts.end() )
    {
        return 0;
    }

    const TestResultsRow& row = iter->second;

    TestResultsRow::const_iterator rowIter = row.find(dst);
    
    if( rowIter == row.end() )
    {
        return 0;
    }

    return rowIter->second;
}

unsigned
TocinoTestResults::GetTotalCount() const
{
    unsigned total = 0;

    TestResults::const_iterator i;
    TestResultsRow::const_iterator j;

    for( i = m_counts.begin(); i != m_counts.end(); i++ ) 
    {
        const TestResultsRow& row = m_counts.find(i->first)->second;

        for( j = row.begin(); j != row.end(); j++ ) 
        {
            total += j->second;
        }
    }

    return total;
}

unsigned
TocinoTestResults::GetTotalBytes() const
{
    unsigned total = 0;
    
    TestResults::const_iterator i;
    TestResultsRow::const_iterator j;
    
    for( i = m_bytes.begin(); i != m_bytes.end(); i++ ) 
    {
        const TestResultsRow& row = m_bytes.find(i->first)->second;

        for( j = row.begin(); j != row.end(); j++ ) 
        {
            total += j->second;
        }
    }

    return total;
}

std::string
TocinoTestResults::ToString() const
{
    TestResults::const_iterator i;
    TestResultsRow::const_iterator j;
    
    std::ostringstream oss;

    for( i = m_counts.begin(); i != m_counts.end(); i++ ) 
    {
        const TestResultsRow& row = m_counts.find(i->first)->second;

        for( j = row.begin(); j != row.end(); j++ ) 
        {
            oss << "TocinoTestResults: "
                << i->first
                << " --> "
                << j->first
                << " = "
                << j->second
                << std::endl;
        }
    }

    return oss.str();
}

}
