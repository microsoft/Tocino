/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>
#include <sstream>

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"
#include "tocino-router.h"
#include "tocino-flit-id-tag.h"

NS_LOG_COMPONENT_DEFINE ("TocinoRx");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->GetTocinoAddress().GetX() << "," \
                << (int) m_tnd->GetTocinoAddress().GetY() << "," \
                << (int) m_tnd->GetTocinoAddress().GetZ() << ") " \
                << m_inputPortNumber << " "; }
#endif

namespace ns3 {

TocinoRx::TocinoRx( 
        const uint32_t portNumber,
        Ptr<TocinoNetDevice> tnd
)
    : m_inputPortNumber( portNumber )
    , m_tnd( tnd )
    , m_tx( tnd->GetTransmitter( portNumber ) )
    , m_crossbar( tnd, this )
{
    m_inputQueues.vec.resize( m_tnd->GetNVCs() );
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        SetInputQueue( inputVC, CreateObject<CallbackQueue>() );
    }
}

uint32_t
TocinoRx::GetPortNumber() const
{
    return m_inputPortNumber;
}

Ptr<NetDevice>
TocinoRx::GetNetDevice()
{ 
    return m_tnd;
}

bool
TocinoRx::IsVCBlocked( const TocinoInputVC inputVC ) const
{
    return GetInputQueue( inputVC )->IsAlmostFull();
}

Ptr<const Packet>
TocinoRx::PeekNextFlit( const TocinoInputVC inputVC ) const
{
    return GetInputQueue( inputVC )->Peek();
}
    
Ptr<CallbackQueue>
TocinoRx::GetInputQueue( const TocinoInputVC inputVC ) const
{
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );

    return m_inputQueues.vec[ inputVC.AsUInt32() ];
}

void
TocinoRx::SetInputQueue(
        const TocinoInputVC inputVC,
        const Ptr<CallbackQueue> queue )
{
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );

    m_inputQueues.vec[ inputVC.AsUInt32() ] = queue;
}

bool
TocinoRx::EnqueueHelper( Ptr<Packet> flit, const TocinoInputVC inputVC )
{
    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( flit ) == inputVC,
        "attempt to enqueue flit with mismatched VC" );
     
    bool wasNotBlocked = !IsVCBlocked( inputVC );

    bool success = GetInputQueue( inputVC )->Enqueue( flit );

    bool isNowBlocked = IsVCBlocked( inputVC );
    
    NS_ASSERT_MSG( success, "Queue overrun? inputVC=" << inputVC );
    
    bool enqueueTriggeredBlock = wasNotBlocked && isNowBlocked;

    return enqueueTriggeredBlock;
}

void
TocinoRx::Receive( Ptr<Packet> flit )
{
    NS_LOG_FUNCTION( GetTocinoFlitIdString( flit ) );
    
    if( IsTocinoFlowControlFlit( flit ) )
    {
        NS_LOG_LOGIC( "got flow control flit" );
       
        TocinoFlowControlState newXState = GetTocinoFlowControlState( flit );
        m_tx->SetXState( newXState );

        return;
    }

    const TocinoInputVC inputVC = GetTocinoFlitVirtualChannel( flit );

    bool blocked = EnqueueHelper( flit, inputVC );

    if( blocked )
    {
        // FIXME:
        // We intend to model an ejection port that can never be full. Yet,
        // we call RemotePause even when tx_port == m_tnd->GetHostPort(),
        // in order to avoid overrunning the ejection port queues.

        if( m_inputPortNumber != m_tnd->GetHostPort() )
        {
            m_tx->RemotePause( inputVC );
        }
    }

    TryForwardFlit();
}

Ptr<Packet>
TocinoRx::DequeueHelper(
        const TocinoInputVC inputVC,
        bool &dequeueTriggeredUnblock )
{
    bool wasBlocked = IsVCBlocked( inputVC );

    Ptr<Packet> flit = GetInputQueue( inputVC )->Dequeue();

    bool isNoLongerBlocked = !IsVCBlocked( inputVC );
    
    NS_ASSERT_MSG( flit != NULL, "Queue underrun? inputVC=" << inputVC );

    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( flit ) == inputVC,
            "Dequeued flit has wrong VC?" );
            
    dequeueTriggeredUnblock = wasBlocked && isNoLongerBlocked;
    
    return flit;
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

void
TocinoRx::TryForwardFlit()
{
    NS_LOG_FUNCTION_NOARGS();
    
    TocinoRoute route = m_crossbar.FindForwardableRoute();

    if( route == TocinoCrossbar::NO_FORWARDABLE_ROUTE )
    {
        NS_LOG_LOGIC( "no forwardable flits" );

        return;
    }

    const TocinoInputVC inputVC = route.inputVC;
    const TocinoOutputVC outputVC = route.outputVC;
    
    bool unblocked = false;

    Ptr<Packet> flit = DequeueHelper( inputVC, unblocked );

    if( inputVC != outputVC )
    {
        // Router has elected to change virtual channels
        RewriteFlitHeaderVC( flit, outputVC );
    }
     
    // Forward the flit along its route
    m_crossbar.ForwardFlit( flit, route );

    // If we just became unblocked ask our corresponding
    // transmitter to resume the remote node
    if( unblocked )
    {
        if( m_inputPortNumber == m_tnd->GetHostPort() )
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
        GetInputQueue( inputVC )->SetFreeWM( numFlits );
    }
}

bool
TocinoRx::AllQuiet() const
{
    bool quiet = true;
    
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        if( !GetInputQueue( inputVC )->IsEmpty() )
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
    NS_LOG_LOGIC("receiver=" << m_inputPortNumber);
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        if( IsVCBlocked( inputVC ) )
        {
            NS_LOG_LOGIC(" inputVC=" << inputVC << " BLOCKED");
            
            Ptr<CallbackQueue> queue = GetInputQueue( inputVC );

            for( uint32_t i = 0; i < queue->Size(); i++ )
            {
                NS_LOG_LOGIC("   " << GetTocinoFlitIdString( queue->At(i) ) );
            }
        }
        else
        {
            NS_LOG_LOGIC(" inputVC=" << inputVC << " not blocked");
        }

        TocinoRoute route = 
            m_crossbar.GetForwardingTable().GetRoute( inputVC );

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
