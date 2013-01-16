/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>
#include <sstream>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

#include "tocino-rx.h"
#include "tocino-net-device.h"
#include "tocino-tx.h"
#include "callback-queue.h"
#include "tocino-misc.h"
#include "tocino-router.h"
#include "tocino-dimension-order-router.h"
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
        Ptr<TocinoNetDevice> tnd, 
        Ptr<TocinoRouter> router
)
    : m_inputPortNumber( portNumber )
    , m_tnd( tnd )
    , m_tx( tnd->GetTransmitter( portNumber ) )
    , m_router( router )
{
    m_inputQueues.vec.resize( m_tnd->GetNVCs() );
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        SetInputQueue( inputVC, CreateObject<CallbackQueue>() );
    }
}

TocinoRx::~TocinoRx()
{}

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
TocinoRx::CanRouteFrom( const TocinoInputVC inputVC ) const
{
    if( GetInputQueue( inputVC )->IsEmpty() )
    {
        return false;
    }

    return true;
}

void
TocinoRx::RewriteFlitHeaderVC(
        Ptr<Packet> f,
        const TocinoOutputVC newVC ) const
{
    TocinoFlitHeader h;
    f->RemoveHeader( h );
   
    const TocinoInputVC currentVC = h.GetVirtualChannel();

    NS_ASSERT_MSG( newVC != currentVC, "Pointless rewrite?" );

    h.SetVirtualChannel( newVC.AsUInt32() );

    f->AddHeader( h );
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

    bool enqueueTriggeredBlock = EnqueueHelper( flit, inputVC );

    if( enqueueTriggeredBlock )
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

    TryRouteFlit();
}

// ISSUE-REVIEW:
// This function essentially moves flits from the input
// stage to the output stage if possible.  Consider
// moving it to it's own module.  Crossbar?
void
TocinoRx::TryRouteFlit()
{
    NS_LOG_FUNCTION_NOARGS();

    NS_ASSERT( m_router != NULL );
    
    // ISSUE-REVIEW: doing this in inputVC order is unfair
    // and will result in starvation of high VCs 
    for( TocinoInputVC inputVC = 0; inputVC < m_tnd->GetNVCs(); ++inputVC )
    {
        if( !CanRouteFrom( inputVC ) )
        {
            continue;
        }

        // Try to route a flit
        TocinoRoute route = m_router->Route( inputVC ); 

        if( route == TocinoRouter::CANNOT_ROUTE )
        {
            // We may not be able to route a flit at this time.
            // 
            // The target output queue may be full.
            //
            // The router may need to switch virtual channels, for
            // example, to implement the dateline algorithm.  If
            // there is already a route in-progress to the same
            // output port and output vc, we cannot proceed.

            NS_LOG_LOGIC( "cannot route flit from inputVC=" << inputVC );
            continue;
        }
       
        bool dequeueTriggeredUnblock = false;

        Ptr<Packet> flit = DequeueHelper( inputVC, dequeueTriggeredUnblock );

        if( inputVC != route.outputVC )
        {
            // Router has elected to change virtual channels
            RewriteFlitHeaderVC( flit, route.outputVC );
        }
       
        // If we just became unblocked ask our corresponding
        // transmitter to resume the remote node
        if( dequeueTriggeredUnblock )
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

        // Move the flit to the proper transmitter and output queue
        TocinoTx* outputTransmitter = m_tnd->GetTransmitter( route.outputPort );
        outputTransmitter->AcceptFlit( m_inputPortNumber, route.outputVC, flit );
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

        TocinoRoute route = m_router->GetCurrentRoute( inputVC );

        if( route != TocinoRouter::INVALID_ROUTE )
        {
            NS_LOG_LOGIC( "  route in-progress to outputPort="
                    << route.outputPort << " outputVC=" << route.outputVC );
        }
    }
#endif
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
