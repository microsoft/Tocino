/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>
#include <string>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/data-rate.h"

#include "callback-queue.h"
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
                << m_portNumber << " "; }
#endif

namespace ns3 {

TocinoTx::TocinoTx( const uint32_t portNumber, Ptr<TocinoNetDevice> tnd, Ptr<TocinoArbiter> arbiter )
    : m_portNumber( portNumber )
    , m_xState( TocinoAllXON )
    , m_remoteXState( TocinoAllXON )
    , m_doUpdateXState( 0 )
    , m_state( IDLE )
    , m_tnd( tnd )
    , m_channel( NULL )
    , m_arbiter( arbiter )
{
    m_queues.resize( m_tnd->GetNPorts() );
    for( uint32_t port = 0; port < m_queues.size(); ++port )
    {
        m_queues[port].resize( m_tnd->GetNVCs() );
        for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
        {
            m_queues[port][vc].resize( m_tnd->GetNVCs() );
        }
    }
}

TocinoTx::~TocinoTx()
{}

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
    return m_portNumber;
}

bool
TocinoTx::IsVCPaused( const uint32_t vc ) const
{
    TocinoVCBitSet xoffBits = ~m_xState;
    return xoffBits[vc];
}

bool
TocinoTx::IsAnyVCPaused() const
{
    TocinoVCBitSet xoffBits = ~m_xState;
    return xoffBits.any();
}

void TocinoTx::SetChannel(Ptr<TocinoChannel> channel)
{
    m_channel = channel;
}

void TocinoTx::RemotePause( const uint8_t vc )
{
    // FIXME: cast required to actually print?!
    NS_LOG_FUNCTION( static_cast<unsigned>(vc) );

    // Asserted bits in m_remoteXState indicate XON.
    // Pausing can ONLY clear bits in m_remoteXState
    
    const TocinoVCBitSet vcMask = ( 1 << vc );
   
    const TocinoVCBitSet unsentResumes = ( m_remoteXState & m_doUpdateXState );

    if( unsentResumes[vc] )
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

void TocinoTx::RemoteResume( const uint8_t vc )
{
    // FIXME: cast required to actually print?!
    NS_LOG_FUNCTION( static_cast<unsigned>(vc) );

    // Asserted bits in m_remoteXState indicate XON.
    // Resuming can ONLY assert bits in m_remoteXState
    
    const TocinoVCBitSet vcMask = ( 1 << vc );
   
    const TocinoVCBitSet unsentPauses = ( ~m_remoteXState & m_doUpdateXState );

    if( unsentPauses[vc] )
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

Ptr<NetDevice> TocinoTx::GetNetDevice()
{
    return m_tnd;
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

    if( m_portNumber == m_tnd->GetHostPort() ) 
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

    if( m_portNumber != m_tnd->GetHostPort() )
    {
        Ptr<Packet> f = GetTocinoFlowControlFlit( m_remoteXState );
        
        NS_LOG_LOGIC( "sending " << m_remoteXState.to_string() );
        
        SendToChannel( f );
    }
        
    m_doUpdateXState.reset();
}

Ptr<Packet>
TocinoTx::DequeueHelper(
        const TocinoQueueDescriptor qd,
        bool &dequeueTriggeredUnblock )
{
    // N.B.
    // 
    // We are about to dequeue from m_queues[...]
    // Afterward, if the receiver is unblocked, the
    // cause is unambiguous: it must be due to our
    // dequeue. Thus, it is tempting to avoid a loop
    // and full scan of all the queues by making the
    // pre-dequeue check into:
    // 
    // wasAlmostfull = m_queues[...]->IsAlmostFull()
    //
    // However, for symmetry (the "blocked" concept
    // seems better here than "almost full"), as well
    // as to avoid premature optimization, we will not
    // do this.
    //
    //  -MAS
    
    bool wasBlocked
        = m_tnd->GetReceiver(qd.port)->IsVCBlocked( qd.inputVC );

    Ptr<Packet> flit = m_queues[qd.port][qd.inputVC][qd.outputVC]->Dequeue();

    bool isNoLongerBlocked
        = !m_tnd->GetReceiver(qd.port)->IsVCBlocked( qd.inputVC );
    
    NS_ASSERT_MSG( flit != NULL, "queue underrun "
            << qd.port << ":"
            << (uint32_t) qd.inputVC << ":"
            << (uint32_t) qd.outputVC );

    NS_ASSERT_MSG( GetTocinoFlitVirtualChannel( flit ) == qd.outputVC, "dequeued flit has wrong VC?" );
            
    dequeueTriggeredUnblock = wasBlocked && isNoLongerBlocked;
    
    return flit;
}

void
TocinoTx::DoTransmit()
{
    NS_LOG_FUNCTION_NOARGS();

    NS_ASSERT( m_arbiter != NULL );

    TocinoQueueDescriptor rx_q = m_arbiter->Arbitrate();

    if( rx_q == TocinoArbiter::DO_NOTHING )
    {
        NS_LOG_LOGIC( "Nothing to do" );
        return;
    }
    
    bool dequeueTriggeredUnblock = false;

    Ptr<Packet> flit = DequeueHelper( rx_q, dequeueTriggeredUnblock );

    SendToChannel( flit );

    // If we just unblocked rx_port, ask the corresponding
    // transmitter to resume the remote node
    if( dequeueTriggeredUnblock )
    {
        if( rx_q.port != m_tnd->GetHostPort() )
        {
            m_tnd->GetTransmitter(rx_q.port)->RemoteResume( rx_q.inputVC );
        }
        else
        {
            // Special handling for injection port
            m_tnd->TrySendFlits();
        }
    }
}

void
TocinoTx::Transmit()
{
    if( m_state == BUSY ) 
    {
        return;
    }
    
    NS_LOG_FUNCTION_NOARGS();
    
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
TocinoTx::CanTransmitFrom(
        const uint32_t inputPort,
        const uint8_t inputVC,
        const uint8_t outputVC ) const
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );
   
    // We can transmit from a queue iff
    //  -It is not empty
    //  -The corresponding output VC is enabled
    
    if( !m_queues[inputPort][inputVC][outputVC]->IsEmpty() ) 
    {
        if( m_xState[outputVC] )
        {
            return true;
        }
    }

    return false;
}

bool
TocinoTx::CanTransmitFrom( const TocinoQueueDescriptor qd ) const
{
    return CanTransmitFrom( qd.port, qd.inputVC, qd.outputVC );
}

Ptr<const Packet>
TocinoTx::PeekNextFlit( 
        const uint32_t inputPort,
        const uint8_t inputVC,
        const uint8_t outputVC ) const
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );
    
    NS_ASSERT( !m_queues[inputPort][inputVC][outputVC]->IsEmpty() );

    return m_queues[inputPort][inputVC][outputVC]->Peek();
}

bool
TocinoTx::IsNextFlitHead( 
        const uint32_t inputPort,
        const uint8_t inputVC,
        const uint8_t outputVC ) const
{
    return IsTocinoFlitHead( PeekNextFlit( inputPort, inputVC, outputVC ) );
}

bool
TocinoTx::IsNextFlitHead( const TocinoQueueDescriptor qd ) const
{
    return IsNextFlitHead( qd.port, qd.inputVC, qd.outputVC ); 
}

bool
TocinoTx::IsNextFlitTail(
        const uint32_t inputPort,
        const uint8_t inputVC,
        const uint8_t outputVC ) const
{
    return IsTocinoFlitTail( PeekNextFlit( inputPort, inputVC, outputVC ) );
}

bool
TocinoTx::IsNextFlitTail( const TocinoQueueDescriptor qd ) const
{
    return IsNextFlitTail( qd.port, qd.inputVC, qd.outputVC ); 
}

void
TocinoTx::SetQueue(
        uint32_t inputPort,
        uint8_t inputVC,
        uint8_t outputVC,
        Ptr<CallbackQueue> q )
{
    NS_ASSERT( inputPort < m_tnd->GetNPorts() );
    NS_ASSERT( inputVC < m_tnd->GetNVCs() );
    NS_ASSERT( outputVC < m_tnd->GetNVCs() );

    m_queues[inputPort][inputVC][outputVC] = q;
}

void
TocinoTx::DumpState()
{
    NS_LOG_LOGIC("transmitter=" << m_portNumber);
    NS_LOG_LOGIC("  xState=" << m_xState);
    for( uint32_t outVC = 0; outVC < m_tnd->GetNVCs(); outVC++ )
    {
        const char* xState_str = (m_xState[outVC])? "XON":"XOFF";
        
        TocinoQueueDescriptor owner = m_arbiter->GetVCOwner( outVC );

        if( owner == TocinoSimpleArbiter::ANY_QUEUE )
        {
            NS_LOG_LOGIC("   outVC=" << outVC << " owner=NONE " << xState_str);

            // can only schedule head flit - which sources have head flit at front of queue
            for( uint32_t inputPort = 0; inputPort < m_tnd->GetNPorts(); inputPort++ )
            {
                for( uint32_t inVC = 0; inVC < m_tnd->GetNVCs(); ++inVC )
                {
                    if (m_queues[inputPort][inVC][outVC]->Size() > 0)
                    {
                        NS_LOG_LOGIC("   next=" << GetTocinoFlitIdString(m_queues[inputPort][inVC][outVC]->At(0)) << 
                                " inputPort=" << inputPort << " inVC=" << inVC );
                    }
                }
            }
        }
        else
        {
            NS_LOG_LOGIC("   outVC=" << outVC << " owning inputPort=" << owner.port << " inVC=" << (uint32_t) owner.inputVC << " " << xState_str);

            if (m_queues[owner.port][owner.inputVC][outVC]->Size())
            {
                NS_LOG_LOGIC("   next=" << GetTocinoFlitIdString(m_queues[owner.port][owner.inputVC][outVC]->At(0)));
            }
            else
            {
                NS_LOG_LOGIC("    empty queue");
            }
        }
    }
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
