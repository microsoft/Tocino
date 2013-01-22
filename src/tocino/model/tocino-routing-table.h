/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ROUTING_TABLE_H__
#define __TOCINO_ROUTING_TABLE_H__

#include <vector>

#include "tocino-misc.h"
#include "tocino-router.h"

namespace ns3
{

class TocinoRoutingTable
{
    public:

    TocinoRoutingTable( size_t size );

    TocinoRoute GetRoute( const TocinoInputVC ) const;
    void InstallRoute( const TocinoInputVC, const TocinoRoute );
    void RemoveRoute( const TocinoInputVC );
   
    private:

    std::vector< TocinoRoute > m_table;
};

}

#endif // __TOCINO_ROUTING_TABLE_H__
