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
        const TocinoNetDevice* tnd,
        const TocinoInputPort inputPort )
    : m_tnd( tnd )
    , m_inputPort( inputPort )
{
    m_forwardingTable.resize( m_tnd->GetNPorts() );
    
    for( TocinoOutputPort outputPort = 0;
         outputPort < m_tnd->GetNPorts(); 
         ++outputPort )
    {
        ForwardingTableVec& vec 
            = m_forwardingTable[ outputPort.AsUInt32() ];

        vec.assign( m_tnd->GetNVCs(), TOCINO_INVALID_VC );
    }
}

const TocinoInputVC&
TocinoCrossbar::GetForwardingTableEntry(
        const TocinoOutputPort outputPort,
        const TocinoOutputVC outputVC ) const
{
    return m_forwardingTable[ outputPort.AsUInt32() ][ outputVC.AsUInt32() ];
}

void
TocinoCrossbar::SetForwardingTableEntry(
        const TocinoOutputPort outputPort,
        const TocinoOutputVC outputVC,
        const TocinoInputVC inputVC )
{
    m_forwardingTable[ outputPort.AsUInt32() ][ outputVC.AsUInt32() ] = inputVC;
}

void
TocinoCrossbar::ResetForwardingTableEntry(
        const TocinoOutputPort outputPort,
        const TocinoOutputVC outputVC )
{
    m_forwardingTable[ outputPort.AsUInt32() ][ outputVC.AsUInt32() ] 
        = TOCINO_INVALID_VC;
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
    
    if( !TransmitterCanAcceptFlit( outputPort, outputVC ) )
    {
        NS_LOG_LOGIC( "can't forward: output queue full" );
        return false;
    }

    const TocinoInputVC currentTableEntry = 
        GetForwardingTableEntry( outputPort, outputVC );

    if( currentTableEntry == TOCINO_INVALID_VC )
    {
        return true;
    }
    
    if( currentTableEntry == inputVC )
    {
        return true;
    }

    NS_LOG_LOGIC( "can't forward: forwarding already in progress" );
    return false;
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
  
    const TocinoInputVC currentTableEntry = 
        GetForwardingTableEntry( outputPort, outputVC );

    if( isHead )
    {
        NS_ASSERT( currentTableEntry == TOCINO_INVALID_VC );

        if( !isTail )
        {
            NS_LOG_LOGIC( "head flit, updating forwarding table" );
            SetForwardingTableEntry( outputPort, outputVC, inputVC );
        }
    }
    else
    {
        NS_ASSERT( currentTableEntry != TOCINO_INVALID_VC );

        if( isTail )
        {
            NS_LOG_LOGIC( "tail flit, clearing forwarding state" );
            ResetForwardingTableEntry( outputPort, outputVC );
        }
    }

    TocinoTx* outputTransmitter = m_tnd->GetTransmitter( outputPort );

    // Forward the flit to the proper transmitter and output queue
    outputTransmitter->AcceptFlit( m_inputPort, outputVC, flit );
}

}
