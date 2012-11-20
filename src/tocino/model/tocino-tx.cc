/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/data-rate.h"

#include "tocino-tx.h"
#include "tocino-rx.h"
#include "callback-queue.h"
#include "tocino-net-device.h"
#include "tocino-channel.h"
#include "tocino-misc.h"
#include "tocino-arbiter.h"

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
    , m_xstate( TocinoFlowControl::XON )
    , m_state( IDLE )
    , m_remoteResumeRequested( false )
    , m_remotePauseRequested( false )
    , m_tnd( tnd )
    , m_queues( tnd->GetNQueues() )
    , m_channel( NULL )
    , m_arbiter( arbiter )
{}

TocinoTx::~TocinoTx()
{}

void
TocinoTx::Pause()
{
    m_xstate = TocinoFlowControl::XOFF;
    NS_LOG_LOGIC("transmitter xstate now XOFF");
}

void
TocinoTx::Resume()
{
    m_xstate = TocinoFlowControl::XON;
    NS_LOG_LOGIC("transmitter xstate now XON");

    Transmit();
}

bool
TocinoTx::IsPaused() const
{
    return m_xstate == TocinoFlowControl::XOFF;
}

void TocinoTx::SetChannel(Ptr<TocinoChannel> channel)
{
    m_channel = channel;
}

void TocinoTx::RemotePause()
{
    // attempts to set both pending flags simultaneously can happen:
    // a buffer fills - generates XOFF - and then is popped - generates XON - 
    // before transmitter becomes !BUSY and can schedule LLC
    if (m_remoteResumeRequested) 
    {
        m_remoteResumeRequested = false;
        NS_LOG_LOGIC("clearing pending XON");
    }
    else
    {
        m_remotePauseRequested = true;
        NS_LOG_LOGIC("pending XOFF");
    }
    
    Transmit();
}

void TocinoTx::RemoteResume()
{
    if (m_remotePauseRequested)
    {
        m_remotePauseRequested = false;
        NS_LOG_LOGIC("clearing pending XOFF");
    }
    else
    {
        m_remoteResumeRequested = true;
        NS_LOG_LOGIC("pending XON");
    }
    
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
        
        // ejection port modeled as having infinite bandwidth and buffering
        // need to keep m_state == BUSY to this point to prevent reentrancy
        m_tnd->EjectFlit( f ); // eject the packet
        TransmitEnd();
    }
    else
    {
        // send packet to channel
        
        NS_ASSERT( m_channel != NULL );

        m_channel->TransmitStart( f );

        Time transmit_time = m_channel->GetTransmissionTime( f );

        NS_LOG_LOGIC("transmitting " << f << " for " << transmit_time);
        Simulator::Schedule(transmit_time, &TocinoTx::TransmitEnd, this);
    }
}


void
TocinoTx::DoTransmitPause()
{
    m_remotePauseRequested = false;

    if( m_portNumber == m_tnd->GetHostPort() )
    {
        // do nothing on an injection port stall
        // this case is handled by the transmitter
        return;
    }

    Ptr<Packet> f = TocinoFlowControl::GetXOFFPacket( 0 );
    
    NS_LOG_LOGIC( "sending XOFF(" << f << ")" );

    // reflect xstate of transmitter on far end of channel in local rx
    // assumption: the rx[i] and tx[i] in this NetDevice connect to some
    // tx[j] and rx[j] on the NetDevice at far end of channel
    // THIS IS VERY IMPORTANT and probably should be checked at construction time
    // an alternative would be to infer the appropriate receiver to signal
    m_tnd->GetReceiver(m_portNumber)->SetUpstreamXState(TocinoFlowControl::XOFF);

    SendToChannel( f );
}

void
TocinoTx::DoTransmitResume()
{
    m_remoteResumeRequested = false;

    if( m_portNumber == m_tnd->GetHostPort() )
    {
        // do nothing on an injection port resume
        // this case is handled by the transmitter
        return;
    }

    Ptr<Packet> f = TocinoFlowControl::GetXONPacket( 0 );
    
    NS_LOG_LOGIC( "sending XON(" << f << ")" );

    // same state reflection as described above
    m_tnd->GetReceiver(m_portNumber)->SetUpstreamXState(TocinoFlowControl::XON);

    SendToChannel( f );
}

void
TocinoTx::DoTransmit()
{
    NS_ASSERT( m_arbiter != NULL );

    uint32_t winner = m_arbiter->Arbitrate();

    if( winner == TocinoArbiter::DO_NOTHING )
    {
        return;
    }
    
    uint32_t rx_port = m_tnd->QueueToPort( winner );

    bool wasBlocked = m_tnd->GetReceiver(rx_port)->IsAnyQueueBlocked();

    // Dequeue must occur whether the receiver was blocked or not 
    Ptr<Packet> f = m_queues[winner]->Dequeue();
    
    NS_ASSERT_MSG( f != NULL, "queue underrun" << winner );

    SendToChannel( f );

    if( wasBlocked )
    {
        if( rx_port == m_tnd->GetHostPort() )
        {
            // Special handling for injection port
            
            m_tnd->TrySendFlits();
        }
        else
        {
            // We may have just unblocked rx_port.
            // If so, CheckForUnblock() will cause
            // a pending XON on the corresponding
            // transmitter
            
            m_tnd->GetReceiver(rx_port)->CheckForUnblock();
        }
    }
}

void
TocinoTx::Transmit()
{
    NS_LOG_FUNCTION_NOARGS();
    
    if( m_state == BUSY ) 
    {
        return;
    }

    NS_ASSERT_MSG( !(m_remotePauseRequested && m_remoteResumeRequested), "race condition detected" );

    if( m_remotePauseRequested )
    {
        DoTransmitPause();
    }
    else if( m_remoteResumeRequested )
    {
        DoTransmitResume();
    }
    else 
    {
        if( m_xstate == TocinoFlowControl::XON )
        {
            // legal to transmit
            DoTransmit();
        }
    }
}

bool
TocinoTx::IsQueueEmpty( uint32_t qnum ) const
{
    NS_ASSERT( qnum < m_queues.size() );

    if( m_queues[qnum]->IsEmpty() ) 
    {
        return true;
    }
    
    return false;
}

bool
TocinoTx::IsQueueNotEmpty( uint32_t qnum ) const
{
    return !IsQueueEmpty( qnum );
}
    
Ptr<const Packet>
TocinoTx::PeekNextFlit( uint32_t qnum ) const
{
    NS_ASSERT( qnum < m_queues.size() );
    NS_ASSERT( !m_queues[qnum]->IsEmpty() );

    return m_queues[qnum]->Peek();
}

bool
TocinoTx::IsNextFlitTail( uint32_t qnum ) const
{
    NS_ASSERT( qnum < m_queues.size() );
    NS_ASSERT( !m_queues[qnum]->IsEmpty() );

    Ptr<const Packet> f = m_queues[qnum]->Peek();
    TocinoFlitHeader h;
    f->PeekHeader( h );

    return h.IsTail();
}

void
TocinoTx::SetQueue( uint32_t qnum, Ptr<CallbackQueue> q )
{
    NS_ASSERT( qnum < m_queues.size() );
    m_queues[qnum] = q;
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
