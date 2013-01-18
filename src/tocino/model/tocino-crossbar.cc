/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/object-factory.h"
#include "ns3/log.h"

#include "tocino-crossbar.h"
#include "tocino-net-device.h"
#include "tocino-router.h"
#include "tocino-rx.h"
#include "tocino-tx.h"
#include "tocino-flit-id-tag.h"

NS_LOG_COMPONENT_DEFINE ("TocinoCrossbar");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->GetTocinoAddress().GetX() << "," \
                << (int) m_tnd->GetTocinoAddress().GetY() << "," \
                << (int) m_tnd->GetTocinoAddress().GetZ() << ") " \
                << m_trx->GetPortNumber() << " "; }
#endif

namespace ns3
{

TocinoCrossbar::TocinoCrossbar(
        Ptr<TocinoNetDevice> tnd,
        TocinoRx* trx )
    : m_tnd( tnd )
    , m_trx( trx )
    , m_forwardingTable( tnd->GetNVCs() )
{
    ObjectFactory routerFactory;
    routerFactory.SetTypeId( m_tnd->GetRouterTypeId() );
    m_router = routerFactory.Create<TocinoRouter>();
    m_router->Initialize( m_tnd, m_trx );
}

const TocinoForwardingTable&
TocinoCrossbar::GetForwardingTable() const
{
    return m_forwardingTable;
}

bool
TocinoCrossbar::ForwardingInProgress( 
        const TocinoInputVC thisInputVC,
        const TocinoOutputPort outputPort,
        const TocinoOutputVC outputVC ) const
{
    for( TocinoInputVC otherInputVC = 0; otherInputVC < m_tnd->GetNVCs(); ++otherInputVC )
    {
        if( otherInputVC == thisInputVC )
        {
            continue;
        }

        TocinoRoute existingRoute = m_forwardingTable.GetRoute( otherInputVC );

        if( ( existingRoute.outputPort == outputPort ) && 
            ( existingRoute.outputVC == outputVC ) )
        {
            return true;
        }
    }

    return false;
}

bool
TocinoCrossbar::TransmitterCanAcceptFlit( 
        const TocinoOutputPort outputPort,
        const TocinoOutputVC outputVC ) const
{
    const uint32_t inputPortNumber = m_trx->GetPortNumber();
    TocinoTx* outputTransmitter = m_tnd->GetTransmitter( outputPort );

    if( outputTransmitter->CanAcceptFlit( inputPortNumber, outputVC ) )
    {
        return true;
    }

    return false;
}
    
// Intentionally distinct from TOCINO_INVALID_ROUTE
const TocinoRoute TocinoCrossbar::NO_FORWARDABLE_ROUTE(
        TOCINO_INVALID_PORT-1, TOCINO_INVALID_VC-1, TOCINO_INVALID_VC-1 );

TocinoRoute
TocinoCrossbar::FindForwardableRoute() const
{
    NS_ASSERT( m_router != NULL );

    // ISSUE-REVIEW: Potential starvation of higher VCs 
    // ISSUE-REVIEW: We may route the same flit many times
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        NS_LOG_LOGIC( "considering inputVC=" << inputVC );
        
        Ptr<const Packet> flit = m_trx->PeekNextFlit( inputVC ); 

        if( flit == NULL )
        {
            NS_LOG_LOGIC( "can't forward: input queue empty" );

            continue;
        }

        TocinoRoute route( TOCINO_INVALID_ROUTE );
        
        std::ostringstream logPrefix;

        if( IsTocinoFlitHead( flit ) )
        {
            // Make a new routing decision
            route = m_router->Route( flit );

            logPrefix << "new route via ";
        }
        else
        {
            // Recall previous routing decision
            route = m_forwardingTable.GetRoute( inputVC );

            logPrefix << "existing route via ";
        }

        NS_ASSERT( route != TOCINO_INVALID_ROUTE );
        
        const TocinoOutputPort outputPort = route.outputPort;
        const TocinoOutputVC outputVC = route.outputVC;

        logPrefix 
            << Tocino3dTorusPortNumberToString( outputPort.AsUInt32() )
            << ", inputVC=" << inputVC
            << ", outputVC=" << inputVC;

        if( ForwardingInProgress( inputVC, outputPort, outputVC ) )
        {
            // Cannot foward this flit at this time.
            //
            // We are already forwarding flits from a 
            // different packet to this output queue.
            
            NS_LOG_LOGIC( "can't forward: forwarding already in progress" );

            continue;
        }
   
        if( !TransmitterCanAcceptFlit( outputPort, outputVC ) )
        {
            NS_LOG_LOGIC( "can't forward: output queue full" );
            
            continue;
        }
    
        NS_LOG_LOGIC( "can forward along this route" );
        
        NS_LOG_LOGIC( logPrefix.str() );

        if( inputVC != outputVC )
        {
            NS_LOG_LOGIC( "route changes VC" );
        }

        return route; 
    }
   
    NS_LOG_LOGIC( "can't forward at this time" );

    return NO_FORWARDABLE_ROUTE;
}

void
TocinoCrossbar::ForwardFlit(
        Ptr<Packet> flit,
        const TocinoRoute route )
{
    NS_ASSERT( route != TOCINO_INVALID_ROUTE );
    NS_ASSERT( route != NO_FORWARDABLE_ROUTE );

    NS_ASSERT( flit != NULL );

    const TocinoOutputPort outputPort = route.outputPort;
    const TocinoInputVC inputVC = route.inputVC;
    const TocinoOutputVC outputVC = route.outputVC;
    
    NS_LOG_FUNCTION( "forwarding "
            << GetTocinoFlitIdString( flit ) 
            << " from inputVC=" << inputVC
            << " to outputPort=" << outputPort
            << ", outputVC=" << outputVC );

    const bool isHead = IsTocinoFlitHead( flit );
    const bool isTail = IsTocinoFlitTail( flit );
  
    const TocinoRoute& currentTableEntry 
        = m_forwardingTable.GetRoute(inputVC);

    // Paranoia: ensure consistency of forwarding table
    if( isHead )
    {
        NS_ASSERT( currentTableEntry == TOCINO_INVALID_ROUTE );
    }
    else
    {
        NS_ASSERT( currentTableEntry != TOCINO_INVALID_ROUTE );
    }

    if( isHead && !isTail )
    {
        NS_LOG_LOGIC( 
            "head flit, installing new route for inputVC=" << inputVC );

        // Update the forwarding table 
        m_forwardingTable.SetRoute( inputVC, route );
    }
    else if( !isHead && isTail )
    {
        NS_LOG_LOGIC(
            "tail flit, clearing state for inputVC=" << inputVC );

        // Tear down routing decision by resetting state on a tail flit.
        m_forwardingTable.ClearRoute( inputVC );
    }
    
    TocinoTx* outputTransmitter = m_tnd->GetTransmitter( outputPort );
    const uint32_t inputPortNumber = m_trx->GetPortNumber();

    // Forward the flit to the proper transmitter and output queue
    outputTransmitter->AcceptFlit( inputPortNumber, outputVC, flit );
}

}
