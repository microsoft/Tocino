/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-routing-table.h"

namespace ns3
{
    
TocinoRoutingTable::TocinoRoutingTable( size_t size )
    : m_table( size )
{}

TocinoRoute
TocinoRoutingTable::GetRoute( const TocinoInputVC inputVC ) const
{
    NS_ASSERT( inputVC < m_table.size() );

    return m_table[ inputVC.AsUInt32() ];
}

void
TocinoRoutingTable::InstallRoute( 
        const TocinoInputVC inputVC,
        const TocinoRoute route )
{
    NS_ASSERT( inputVC < m_table.size() );
    NS_ASSERT( route.inputVC == inputVC );
    
    m_table[ inputVC.AsUInt32() ] = route;
}

void
TocinoRoutingTable::RemoveRoute( 
        const TocinoInputVC inputVC )
{
    NS_ASSERT( inputVC < m_table.size() );

    m_table[ inputVC.AsUInt32() ] = TOCINO_INVALID_ROUTE;
}

}
