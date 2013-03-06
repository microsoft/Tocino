/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>
#include <string>

#include "ns3/assert.h"
#include "ns3/data-rate.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include "tocino-arbiter.h"
#include "tocino-channel.h"
#include "tocino-flit-id-tag.h"
#include "tocino-misc.h"
#include "tocino-net-device.h"
#include "tocino-rx.h"
#include "tocino-simple-arbiter.h"
#include "tocino-tx.h"

NS_LOG_COMPONENT_DEFINE ("TocinoTx");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->GetTocinoAddress().GetX() << "," \
                << (int) m_tnd->GetTocinoAddress().GetY() << "," \
                << (int) m_tnd->GetTocinoAddress().GetZ() << ") " \
                << m_outputPort << " "; }
#endif

namespace ns3 {

TocinoTx::TocinoTx(
        const TocinoOutputPort outputPort,
        TocinoNetDevice* tnd
)
    : m_outputPort( outputPort )
    , m_xState( TocinoAllXON )
    , m_remoteXState( TocinoAllXON )
    , m_doUpdateXState( 0 )
    , m_state( IDLE )
    , m_tnd( tnd )
    , m_channel( NULL )
{
    m_outputQueues.vec.resize( m_tnd->GetNPorts() );
    for( TocinoInputPort inputPort = 0; inputPort < m_tnd->GetNPorts(); ++inputPort )
    {
        m_outputQueues.vec[ inputPort.AsUInt32() ].resize( m_tnd->GetNVCs() );
    }
    
    ObjectFactory arbiterFactory;
    arbiterFactory.SetTypeId( m_tnd->GetArbiterTypeId() );
    m_arbiter = arbiterFactory.Create<TocinoArbiter>();
    m_arbiter->Initialize( m_tnd, this );
}

TocinoTx::OutputQueue&
TocinoTx::GetOutputQueue( 
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) 
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );

    return m_outputQueues.vec[ inputPort.AsUInt32() ][ outputVC.AsUInt32() ];
}

const TocinoTx::OutputQueue&
TocinoTx::GetOutputQueue( 
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) const
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );

    return m_outputQueues.vec[ inputPort.AsUInt32() ][ outputVC.AsUInt32() ];
}

void
TocinoTx::SetXState( const TocinoFlowControlState& newXState )
{
    NS_LOG_LOGIC( "set local XState to " << newXState.to_string() );

    // If we are resuming any VCs, we should kick transmit
    bool shouldTransmit = ( ~m_xState & newXState ).any();

    m_xState = newXState;

    if( shouldTransmit )
    {
        Transmit();
    }
}

uint32_t
TocinoTx::GetPortNumber() const
{
    return m_outputPort.AsUInt32();
}

bool
TocinoTx::IsVCPaused( const TocinoOutputVC outputVC ) const
{
    TocinoVCBitSet xoffBits = ~m_xState;
    return xoffBits[ outputVC.AsUInt32() ];
}

void TocinoTx::SetChannel(Ptr<TocinoChannel> channel)
{
    m_channel = channel;
}

Ptr<TocinoChannel>
TocinoTx::GetChannel() const
{
    return m_channel;
}

void TocinoTx::RemotePause( const TocinoInputVC inputVC )
{
    NS_LOG_FUNCTION( inputVC );

    // Asserted bits in m_remoteXState indicate XON.
    // Pausing can ONLY clear bits in m_remoteXState
    
    const TocinoVCBitSet vcMask = ( 1 << inputVC.AsUInt32() );
   
    const TocinoVCBitSet unsentResumes = ( m_remoteXState & m_doUpdateXState );

    if( unsentResumes[ inputVC.AsUInt32() ] )
    {
        // We had an unsent resume on this VC

        // Simply clear remoteXState bit to indicate XOFF
        m_remoteXState &= ~vcMask;

        // No longer need to update this VC
        m_doUpdateXState &= ~vcMask;
        
        return;
    }
   
    TocinoVCBitSet orig( m_remoteXState );

    // We need to pause this VC
    m_remoteXState &= ~vcMask;

    NS_ASSERT( m_remoteXState != orig );

    m_doUpdateXState |= vcMask;

    Transmit();
}

void TocinoTx::RemoteResume( const TocinoInputVC inputVC )
{
    NS_LOG_FUNCTION( inputVC );

    // Asserted bits in m_remoteXState indicate XON.
    // Resuming can ONLY assert bits in m_remoteXState
    
    const TocinoVCBitSet vcMask = ( 1 << inputVC.AsUInt32() );
   
    const TocinoVCBitSet unsentPauses = ( ~m_remoteXState & m_doUpdateXState );

    if( unsentPauses[ inputVC.AsUInt32() ] )
    {
        // We had an unsent pause on this VC
        
        // Simply assert remoteXState bit to indicate XON
        m_remoteXState |= vcMask;

        // No longer need to update this VC
        m_doUpdateXState &= ~vcMask;

        return;
    }
    
    TocinoVCBitSet orig( m_remoteXState );
    
    // We need to resume this VC
    m_remoteXState |= vcMask;
    
    NS_ASSERT( m_remoteXState != orig );

    m_doUpdateXState |= vcMask;

    Transmit();
}

Ptr<NetDevice>
TocinoTx::GetNetDevice()
{
    return Ptr<NetDevice>( m_tnd );
}

Ptr<TocinoNetDevice>
TocinoTx::GetTocinoNetDevice()
{ 
    return Ptr<TocinoNetDevice>( m_tnd );
}

void
TocinoTx::TransmitEnd()
{
  m_state = IDLE;
  Transmit();
}

void
TocinoTx::SendToChannel( Ptr<Packet> f )
{
    // this acts as a mutex on Transmit
    m_state = BUSY;

    if( m_outputPort == m_tnd->GetHostPort() ) 
    {
        // ejection port
        NS_LOG_LOGIC( "ejecting " << GetTocinoFlitIdString( f ) );
        
        // ejection port modeled as having infinite bandwidth and buffering
        // need to keep m_state == BUSY to this point to prevent reentrancy
        m_tnd->EjectFlit( f ); // eject the packet

        // ScheduleNow here, rather than direct call to TransmitEnd
        // avoids mind-bending reentrancy due to:
        //    Transmit -> SendToChannel -> TransmitEnd -> Transmit
        // Otherwise we can end up with multiple Transmits in flight
        // at once, which is very confusing.
        
        Simulator::ScheduleNow( &TocinoTx::TransmitEnd, this );
    }
    else
    {
        // send packet to channel
        NS_ASSERT( m_channel != NULL );
        m_channel->TransmitStart( f );
        Time transmit_time = m_channel->GetTransmissionTime( f );

        NS_LOG_LOGIC( "transmitting " << GetTocinoFlitIdString( f ) 
            << " for " << transmit_time );

        Simulator::Schedule(transmit_time, &TocinoTx::TransmitEnd, this);
    }
}


void
TocinoTx::DoTransmitFlowControl()
{
    NS_LOG_FUNCTION_NOARGS();

    if( m_outputPort != m_tnd->GetHostPort() )
    {
        Ptr<Packet> f = GetTocinoFlowControlFlit( m_remoteXState );
        
        NS_LOG_LOGIC( "sending " << m_remoteXState.to_string() );
        
        SendToChannel( f );
    }
        
    m_doUpdateXState.reset();
}

void
TocinoTx::DoTransmit()
{
    NS_LOG_FUNCTION_NOARGS();

    NS_ASSERT( m_arbiter != NULL );

    TocinoArbiterAllocation winner = m_arbiter->Arbitrate();

    if( winner == TocinoArbiter::DO_NOTHING )
    {
        NS_LOG_LOGIC( "nothing to do" );
        return;
    }
  
    Ptr<Packet> flit =
        GetOutputQueue( winner.inputPort, winner.outputVC ).Dequeue();

    NS_ASSERT_MSG( flit != NULL, "Queue underrun? inputPort="
            << winner.inputPort << " outputVC=" << winner.outputVC );

    SendToChannel( flit );

    // Give the inputPort an opportunity to push another flit
    m_tnd->GetReceiver( winner.inputPort )->TryForwardFlit();
}

void
TocinoTx::Transmit()
{
    NS_LOG_FUNCTION_NOARGS();

    if( m_state == BUSY ) 
    {
        NS_LOG_LOGIC( "transmitter busy" );
        return;
    }
    
    if( m_doUpdateXState.any() )
    {
        DoTransmitFlowControl();
    }
    else 
    {
        DoTransmit();
    }
}

bool
TocinoTx::CanAcceptFlit(
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) const
{
    return !GetOutputQueue( inputPort, outputVC ).IsFull();
}

void
TocinoTx::AcceptFlit(
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC,
        Ptr<Packet> flit )
{
    NS_LOG_LOGIC( "inputPort=" << inputPort << " outputVC="
            << outputVC << " " << GetTocinoFlitIdString( flit ) );

    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );
  
    NS_ASSERT( CanAcceptFlit( inputPort, outputVC ) );

    GetOutputQueue( inputPort, outputVC ).Enqueue( flit );

    // Kick off transmission
    Transmit();
}

bool
TocinoTx::IsQueueEmpty(
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) const
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );
   
    return GetOutputQueue( inputPort, outputVC ).IsEmpty();
}


Ptr<const Packet>
TocinoTx::PeekNextFlit( 
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) const
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );
    
    NS_ASSERT( !GetOutputQueue( inputPort, outputVC ).IsEmpty() );

    return GetOutputQueue( inputPort, outputVC ).PeekFront();
}

bool
TocinoTx::IsNextFlitHead( 
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) const
{
    return IsTocinoFlitHead( PeekNextFlit( inputPort, outputVC ) );
}

bool
TocinoTx::IsNextFlitTail(
        const TocinoInputPort inputPort,
        const TocinoOutputVC outputVC ) const
{
    return IsTocinoFlitTail( PeekNextFlit( inputPort, outputVC ) );
}

bool
TocinoTx::AllQuiet() const
{
    bool quiet = true;
   
    for( TocinoInputPort inputPort = 0; inputPort < m_tnd->GetNPorts(); ++inputPort )
    {
        for( TocinoOutputVC outputVC = 0; outputVC < m_tnd->GetNVCs(); ++outputVC )
        { 
            if( IsVCPaused( outputVC ) )
            {
                NS_LOG_LOGIC( "Not quiet: outputVC=" << outputVC << " is XOFF" );
                quiet = false;
            }

            if( !GetOutputQueue( inputPort, outputVC ).IsEmpty() )
            {
                NS_LOG_LOGIC( "Not quiet: "
                        << "m_outputQueues" 
                        << "[" << inputPort << "]" 
                        << "[" << outputVC << "]" 
                        << " not empty" );

                quiet = false;
            }
        }
    }

    return quiet;
}

void
TocinoTx::DumpState() const
{
#ifdef NS3_LOG_ENABLE
    NS_LOG_LOGIC("transmitter=" << m_outputPort);
    NS_LOG_LOGIC("  xState=" << m_xState);
    for( TocinoOutputVC outputVC = 0; outputVC < m_tnd->GetNVCs(); outputVC++ )
    {
        const char* xState_str = IsVCPaused( outputVC ) ? "XOFF" : "XON";
        
        TocinoArbiterAllocation owner = m_arbiter->GetVCOwner( outputVC );

        if( owner == TocinoSimpleArbiter::ANY_QUEUE )
        {
            NS_LOG_LOGIC("   outputVC=" << outputVC << " owner=NONE " << xState_str);

            // can only schedule head flit - which sources have head flit at front of queue
            for( TocinoInputPort inputPort = 0; inputPort < m_tnd->GetNPorts(); inputPort++ )
            {
                const OutputQueue& queue = GetOutputQueue( inputPort, outputVC );

                if( !queue.IsEmpty() )
                {
                    NS_LOG_LOGIC("   next="
                            << GetTocinoFlitIdString( queue.PeekFront() )
                            << " inputPort=" << inputPort );
                }
            }
        }
        else
        {
            NS_LOG_LOGIC( "   outputVC=" << owner.outputVC << " owning inputPort=" 
                    << owner.inputPort << " " << xState_str );
            
            const OutputQueue& queue = 
                GetOutputQueue( owner.inputPort, owner.outputVC );

            if( queue.IsEmpty() )
            {
                NS_LOG_LOGIC("    empty queue");
            }
            else
            {
                NS_LOG_LOGIC( "   next=" << GetTocinoFlitIdString( queue.PeekFront() ) );
            }
        }
    }
#endif
}

void
TocinoTx::ReportStatistics() const
{
    if( m_arbiter != NULL )
    {
        m_arbiter->ReportStatistics();
    }

    if( m_channel != NULL )
    { 
        m_channel->ReportStatistics();
    }
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
