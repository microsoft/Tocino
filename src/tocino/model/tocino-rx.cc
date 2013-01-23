/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>
#include <sstream>

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "tocino-channel.h"
#include "tocino-flit-id-tag.h"
#include "tocino-net-device.h"
#include "tocino-rx.h"
#include "tocino-tx.h"

NS_LOG_COMPONENT_DEFINE ("TocinoRx");

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

namespace ns3 {

TocinoRx::TocinoRx( 
        const uint32_t inputPortNumber,
        Ptr<TocinoNetDevice> tnd
)
    : m_inputPort( inputPortNumber )
    , m_tnd( tnd )
    , m_tx( tnd->GetTransmitter( inputPortNumber ) )
    , m_routingTable( tnd->GetNVCs() )
    , m_crossbar( tnd, inputPortNumber )
{
    m_inputQueues.vec.resize( m_tnd->GetNVCs() );
    
    ObjectFactory routerFactory;
    routerFactory.SetTypeId( m_tnd->GetRouterTypeId() );
    m_router = routerFactory.Create<TocinoRouter>();
    m_router->Initialize( m_tnd, inputPortNumber );
}

uint32_t
TocinoRx::GetPortNumber() const
{
    return m_inputPort.AsUInt32();
}

Ptr<NetDevice>
TocinoRx::GetNetDevice()
{ 
    return m_tnd;
}

void
TocinoRx::SetChannel( Ptr<TocinoChannel> chan )
{
    m_channel = chan;
    
    const uint32_t reserve = chan->FlitBuffersRequired();
    SetReserveFlits( reserve );
}

bool
TocinoRx::IsVCBlocked( const TocinoInputVC inputVC ) const
{
    return GetInputQueue( inputVC ).IsAlmostFull();
}

TocinoRx::InputQueue&
TocinoRx::GetInputQueue( const TocinoInputVC inputVC )
{
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );

    return m_inputQueues.vec[ inputVC.AsUInt32() ];
}

const TocinoRx::InputQueue&
TocinoRx::GetInputQueue( const TocinoInputVC inputVC ) const
{
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );

    return m_inputQueues.vec[ inputVC.AsUInt32() ];
}

bool
TocinoRx::EnqueueHelper(
        const InputQueueEntry& qe,
        const TocinoInputVC inputVC )
{
    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( qe.flit ) == inputVC,
        "attempt to enqueue flit with mismatched VC" );
     
    bool wasNotBlocked = !IsVCBlocked( inputVC );

    GetInputQueue( inputVC ).Enqueue( qe );

    bool isNowBlocked = IsVCBlocked( inputVC );
    
    bool enqueueTriggeredBlock = wasNotBlocked && isNowBlocked;

    return enqueueTriggeredBlock;
}

void
TocinoRx::AnnounceRoutingDecision(
        Ptr<const Packet> flit,
        const TocinoRoute& route ) const
{
    std::ostringstream logPrefix;

    if( IsTocinoFlitHead( flit ) )
    {
        logPrefix << "new route via ";
    }
    else
    {
        logPrefix << "existing route via ";
    }
    
    const TocinoInputVC inputVC = route.inputVC;
    const TocinoOutputPort outputPort = route.outputPort;
    const TocinoOutputVC outputVC = route.outputVC;

    NS_LOG_LOGIC( logPrefix.str()
            << Tocino3dTorusPortNumberToString( outputPort )
            << " (outputPort=" << outputPort
            << ", inputVC=" << inputVC
            << ", outputVC=" << outputVC << ")" );
}

const TocinoRoute
TocinoRx::MakeRoutingDecision( Ptr<const Packet> flit )
{
    const TocinoInputVC inputVC = GetTocinoFlitVirtualChannel( flit );
    
    const bool isHead = IsTocinoFlitHead( flit );
    const bool isTail = IsTocinoFlitTail( flit );

    TocinoRoute route( TOCINO_INVALID_ROUTE );

    if( isHead )
    {
        route = m_router->Route( flit );

        if( !isTail )
        {
            m_routingTable.InstallRoute( inputVC, route );
        }
    }
    else
    {
        route = m_routingTable.GetRoute( inputVC );
    }
        
    NS_ASSERT( route != TOCINO_INVALID_ROUTE );

    if( isTail && !isHead )
    {
        NS_LOG_LOGIC( "removing route for inputVC=" << inputVC );
        m_routingTable.RemoveRoute( inputVC );
    }

    return route;
}

void
TocinoRx::Receive( Ptr<Packet> flit )
{
    NS_LOG_FUNCTION( GetTocinoFlitIdString( flit ) );
    
    NS_ASSERT( m_router != NULL );
    
    if( IsTocinoFlowControlFlit( flit ) )
    {
        NS_LOG_LOGIC( "got flow control flit" );
        TocinoFlowControlState newXState = GetTocinoFlowControlState( flit );
        m_tx->SetXState( newXState );

        return;
    }

    const TocinoInputVC inputVC = GetTocinoFlitVirtualChannel( flit );

    const TocinoRoute route = MakeRoutingDecision( flit );

    AnnounceRoutingDecision( flit, route );

    bool blocked = EnqueueHelper( InputQueueEntry( flit, route ), inputVC );
    
    if( blocked )
    {
        // FIXME:
        // We intend to model an ejection port that can never be full. Yet,
        // we call RemotePause even when tx_port == m_tnd->GetHostPort(),
        // in order to avoid overrunning the ejection port queues.

        if( m_inputPort != m_tnd->GetHostPort() )
        {
            m_tx->RemotePause( inputVC );
        }
    }

    TryForwardFlit();
}

const TocinoRx::InputQueueEntry
TocinoRx::DequeueHelper(
        const TocinoInputVC inputVC,
        bool &dequeueTriggeredUnblock )
{
    bool wasBlocked = IsVCBlocked( inputVC );

    const InputQueueEntry qe = GetInputQueue( inputVC ).Dequeue();

    bool isNoLongerBlocked = !IsVCBlocked( inputVC );
    
    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( qe.flit ) == inputVC,
            "Dequeued flit has wrong VC?" );
            
    dequeueTriggeredUnblock = wasBlocked && isNoLongerBlocked;
    
    return qe;
}

void
TocinoRx::RewriteFlitHeaderVC(
        Ptr<Packet> flit,
        const TocinoOutputVC newVC ) const
{
    TocinoFlitHeader h;
    flit->RemoveHeader( h );
   
    const TocinoInputVC currentVC = h.GetVirtualChannel();

    NS_ASSERT_MSG( newVC != currentVC, "Pointless rewrite?" );

    h.SetVirtualChannel( newVC.AsUInt32() );

    flit->AddHeader( h );
}

// Intentionally distinct from TOCINO_INVALID_VC
const TocinoInputVC TocinoRx::NO_FORWARDABLE_VC( TOCINO_INVALID_VC-1 );

TocinoInputVC
TocinoRx::FindForwardableVC() const
{
    NS_LOG_FUNCTION_NOARGS();
    
    NS_ASSERT( m_router != NULL );

    // ISSUE-REVIEW: Potential starvation of higher VCs 
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        const InputQueue& queue = GetInputQueue( inputVC );

        if( queue.IsEmpty() )
        {
            continue;
        }
        
        const InputQueueEntry& qe = queue.PeekFront();

        if( !m_crossbar.IsForwardable( qe.route ) )
        {
            // This VC is not currently forwardable
            continue;
        }
            
        return inputVC;
    }

    return NO_FORWARDABLE_VC;
}

void
TocinoRx::TryForwardFlit()
{
    NS_LOG_FUNCTION_NOARGS();
    
    NS_ASSERT( m_router != NULL );

    const TocinoInputVC inputVC = FindForwardableVC();

    if( inputVC == NO_FORWARDABLE_VC )
    {
        NS_LOG_LOGIC( "no forwardable inputVC" );
        return;
    }
    
    bool unblocked = false;

    const InputQueueEntry qe = DequeueHelper( inputVC, unblocked );
    
    NS_ASSERT( qe.route.inputVC == inputVC );

    const TocinoOutputVC outputVC = qe.route.outputVC;

    if( inputVC != outputVC )
    {
        NS_LOG_LOGIC( "note: route changes VC" );
        RewriteFlitHeaderVC( qe.flit, outputVC );
    }
    
    // Forward the flit along its route
    m_crossbar.ForwardFlit( qe.flit, qe.route );
    
    // If we just became unblocked ask our corresponding
    // transmitter to resume the remote node
    if( unblocked )
    {
        if( m_inputPort == m_tnd->GetHostPort() )
        {
            // Special handling for injection port

            // ScheduleNow here, rather than direct call to
            // TrySendFlits, avoids mind-bending reentrancy due to:
            //    Recieve -> 
            //      TryRouteFlit -> 
            //        TrySendFlits -> 
            //          InjectFlits -> 
            //            Receive
            // Otherwise we can end up with multiple Receives in
            // flight at once, which is very confusing.

            Simulator::ScheduleNow( &TocinoNetDevice::TrySendFlits, m_tnd );
        }
        else
        {
            m_tx->RemoteResume( inputVC );
        }
    }
}

void
TocinoRx::SetReserveFlits( uint32_t numFlits )
{
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        GetInputQueue( inputVC ).SetReserve( numFlits );
    }
}

bool
TocinoRx::AllQuiet() const
{
    bool quiet = true;
    
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        if( !GetInputQueue( inputVC ).IsEmpty() )
        {
            NS_LOG_LOGIC( "Not quiet: "
                    << "m_inputQueues" 
                    << "[" << inputVC << "]" 
                    << " not empty" );

            quiet = false;
        }
    }

    return quiet;
}

void
TocinoRx::DumpState() const
{
#ifdef NS3_LOG_ENABLE
    NS_LOG_LOGIC("receiver=" << m_inputPort);
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        if( IsVCBlocked( inputVC ) )
        {
            NS_LOG_LOGIC(" inputVC=" << inputVC << " BLOCKED");
            
            const InputQueue& queue = GetInputQueue( inputVC );

            for( uint32_t i = 0; i < queue.Size(); i++ )
            {
                Ptr<const Packet> flit = queue.At(i).flit;

                NS_LOG_LOGIC("   " << GetTocinoFlitIdString( flit ) );
            }
        }
        else
        {
            NS_LOG_LOGIC(" inputVC=" << inputVC << " not blocked");
        }

        TocinoRoute route = m_routingTable.GetRoute( inputVC );

        if( route != TOCINO_INVALID_ROUTE )
        {
            NS_LOG_LOGIC( "  route in-progress to outputPort="
                    << route.outputPort << " outputVC=" << route.outputVC );
        }
    }
#endif
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
