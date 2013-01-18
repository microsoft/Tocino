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
                << m_inputPort << " "; }
#endif

namespace ns3
{

TocinoCrossbar::TocinoCrossbar(
        Ptr<TocinoNetDevice> tnd,
        const TocinoInputPort inputPort )
    : m_tnd( tnd )
    , m_inputPort( inputPort )
    , m_forwardingTable( tnd->GetNVCs() )
{}

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
    for( TocinoInputVC otherInputVC = 0; 
         otherInputVC < m_tnd->GetNVCs();
         ++otherInputVC )
    {
        if( otherInputVC == thisInputVC )
        {
            continue;
        }

        TocinoRoute existingRoute =
            m_forwardingTable.GetRoute( otherInputVC );

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
    TocinoTx* outputTransmitter = m_tnd->GetTransmitter( outputPort );

    if( outputTransmitter->CanAcceptFlit( m_inputPort, outputVC ) )
    {
        return true;
    }

    return false;
}
    
bool
TocinoCrossbar::IsForwardable(
        const TocinoRoute route ) const
{
    NS_ASSERT( route != TOCINO_INVALID_ROUTE );

    const TocinoOutputPort outputPort = route.outputPort;
    const TocinoInputVC inputVC = route.inputVC;
    const TocinoOutputVC outputVC = route.outputVC;

    if( ForwardingInProgress( inputVC, outputPort, outputVC ) )
    {
        NS_LOG_LOGIC( "can't forward: forwarding already in progress" );
        return false;
    }

    if( !TransmitterCanAcceptFlit( outputPort, outputVC ) )
    {
        NS_LOG_LOGIC( "can't forward: output queue full" );
        return false;
    }

    NS_LOG_LOGIC( "can forward along this route" );
    return true;
}

void
TocinoCrossbar::ForwardFlit(
        Ptr<Packet> flit,
        const TocinoRoute route )
{
    NS_ASSERT( flit != NULL );
    NS_ASSERT( route != TOCINO_INVALID_ROUTE );
    NS_ASSERT( IsForwardable( route ) );

    const TocinoOutputPort outputPort = route.outputPort;
    const TocinoInputVC inputVC = route.inputVC;
    const TocinoOutputVC outputVC = route.outputVC;
    
    NS_LOG_LOGIC( "forwarding "
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

    // Forward the flit to the proper transmitter and output queue
    outputTransmitter->AcceptFlit( m_inputPort, outputVC, flit );
}

}
