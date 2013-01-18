/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_FORWARDING_TABLE_H__
#define __TOCINO_FORWARDING_TABLE_H__

#include <vector>

#include "tocino-misc.h"

namespace ns3
{

class TocinoRoute;

class TocinoForwardingTable
{
    public:

    TocinoForwardingTable( size_t size );

    TocinoRoute GetRoute( const TocinoInputVC ) const;
    void ClearRoute( const TocinoInputVC );
    void SetRoute( const TocinoInputVC, const TocinoRoute );
    
    private:

    std::vector< TocinoRoute > m_table;
};

}

#endif // __TOCINO_FORWARDING_TABLE_H__
