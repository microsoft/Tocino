/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-simple-arbiter.h"

#include <vector>

#include "ns3/log.h"
#include "ns3/random-variable.h"

#include "tocino-misc.h"
#include "tocino-tx.h"

NS_LOG_COMPONENT_DEFINE ("TocinoSimpleArbiter");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->GetTocinoAddress().GetX() << "," \
                << (int) m_tnd->GetTocinoAddress().GetY() << "," \
                << (int) m_tnd->GetTocinoAddress().GetZ() << ") " \
                << m_ttx->GetPortNumber() << " "; }
#endif

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (TocinoSimpleArbiter);

TypeId TocinoSimpleArbiter::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoSimpleArbiter" )
        .SetParent<TocinoArbiter>()
        .AddConstructor<TocinoSimpleArbiter>();
    return tid;
}
   
TocinoSimpleArbiter::TocinoSimpleArbiter()
    : m_tnd( NULL )
    , m_ttx( NULL )
#ifdef TOCINO_VC_STRESS_MODE
    , m_lastVC( TOCINO_INVALID_VC )
#endif
{}

void TocinoSimpleArbiter::Initialize( Ptr<TocinoNetDevice> tnd, const TocinoTx* ttx )
{
    m_tnd = tnd;
    m_ttx = ttx;
    m_legalPort.assign( m_tnd->GetNVCs(), ANY_PORT );
}

TocinoSimpleArbiter::QueueVector
TocinoSimpleArbiter::BuildCandidateSet() const
{
    NS_ASSERT( m_tnd->GetNVCs() == m_legalPort.size() );

    QueueVector candidates;

    for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
    {
        if( m_legalPort[vc] == ANY_PORT )
        {
            // can select any port that is ready
            for( uint32_t port = 0; port < m_tnd->GetNPorts(); ++port )
            {
                if( m_ttx->CanTransmitFrom( port, vc ) )
                {
                    candidates.push_back( TocinoQueueDescriptor( port, vc ) );
                }
            }
        }
        else
        {
            // can only select the allocated port, if ready
            uint32_t port = m_legalPort[vc];

            if( m_ttx->CanTransmitFrom( port, vc ) )
            {
                candidates.push_back( TocinoQueueDescriptor( port, vc ) );
            }
        }
    }

    return candidates;
}

TocinoQueueDescriptor
TocinoSimpleArbiter::FairSelectWinner( const QueueVector& cand ) const
{
    UniformVariable rv;

#ifndef TOCINO_VC_STRESS_MODE
    return cand[ rv.GetInteger( 0, cand.size()-1 ) ];
#else
    // try to select a different vc than last time
    TocinoQueueDecriptor winner;
    QueueVector newCand;
   
    if( m_lastVC != TOCINO_INVALID_VC )
    {
        for( unsigned i = 0; i < cand.size(); ++i )
        {
            uint8_t vc = cand[i].vc;

            if( vc != m_lastVC )
            {
                newCand.push_back( cand[i] );
            }
        }
    }

    if( newCand.empty() )
    {
        winner = cand[ rv.GetInteger( 0, cand.size()-1 ) ];
    }
    else
    {
        NS_LOG_LOGIC( "intentional VC interleaving" );
        winner = newCand[ rv.GetInteger( 0, newCand.size()-1 ) ];
    }
    
    m_lastVC = winner.vc;
    return winner;
#endif 
}

void
TocinoSimpleArbiter::UpdateState( const TocinoQueueDescriptor winner )
{
    if( m_ttx->IsNextFlitTail( winner ) )
    {
        // Flow ending, reset
        m_legalPort[ winner.vc ] = ANY_PORT;
    }
    else
    {
        if (m_ttx->IsNextFlitHead( winner ) )
        {
            NS_LOG_LOGIC( "vc=" << (uint32_t) winner.vc 
                << " assigned to src=" << winner.port );
        }
        // Remember mapping
        m_legalPort[ winner.vc ] = winner.port; 
    }
}

TocinoQueueDescriptor
TocinoSimpleArbiter::Arbitrate()
{
    QueueVector candidates = BuildCandidateSet();

    if( candidates.empty() )
    {
        NS_LOG_LOGIC( "No candidates" );
        return DO_NOTHING;
    }

    TocinoQueueDescriptor winner = FairSelectWinner( candidates );

    UpdateState( winner );

    NS_LOG_LOGIC( "Winner is " << winner.port << ":" << winner.vc );

    return winner;
}

uint32_t
TocinoSimpleArbiter::GetVCOwner( const uint8_t vc)
{
    return m_legalPort[vc];
}
const uint32_t TocinoSimpleArbiter::ANY_PORT = TOCINO_INVALID_PORT-1;

}
