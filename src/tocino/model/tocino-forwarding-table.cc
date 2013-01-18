/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "tocino-forwarding-table.h"
#include "tocino-router.h"

namespace ns3
{
    
TocinoForwardingTable::TocinoForwardingTable( size_t size )
    : m_table( size, TOCINO_INVALID_ROUTE )
{}

TocinoRoute
TocinoForwardingTable::GetRoute( const TocinoInputVC inputVC ) const
{
    NS_ASSERT( inputVC < m_table.size() );
    return m_table[ inputVC.AsUInt32() ];
}

void
TocinoForwardingTable::SetRoute( 
        const TocinoInputVC inputVC,
        const TocinoRoute route )
{
    NS_ASSERT( inputVC < m_table.size() );
    NS_ASSERT( route.inputVC == inputVC );
    
    m_table[ inputVC.AsUInt32() ] = route;
}

void
TocinoForwardingTable::ClearRoute( 
        const TocinoInputVC inputVC )
{
    NS_ASSERT( inputVC < m_table.size() );
    m_table[ inputVC.AsUInt32() ] = TOCINO_INVALID_ROUTE;
}

}
