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
    m_legalQueue.assign( m_tnd->GetNVCs(), ANY_QUEUE );
}

TocinoSimpleArbiter::QueueVector
TocinoSimpleArbiter::BuildCandidateSet() const
{
    NS_ASSERT( m_tnd->GetNVCs() == m_legalQueue.size() );

    QueueVector candidates;

    for( uint8_t outVC = 0; outVC < m_tnd->GetNVCs(); ++outVC )
    {
        if( m_legalQueue[outVC] == ANY_QUEUE )
        {
            // can select any queue that is ready
            for( uint32_t outPort = 0; outPort < m_tnd->GetNPorts(); ++outPort )
            {
                for( uint8_t inVC = 0; inVC < m_tnd->GetNVCs(); ++inVC )
                {
                    TocinoQueueDescriptor qd( outPort, inVC, outVC );

                    if( m_ttx->CanTransmitFrom( qd ) )
                    {
                        NS_ASSERT( m_ttx->IsNextFlitHead( qd ) );

                        candidates.push_back( qd );
                    }
                }

            }
        }
        else
        {
            // can only select the allocated queue, if ready
            TocinoQueueDescriptor qd = m_legalQueue[outVC];

            NS_ASSERT( m_legalQueue[outVC].outputVC == outVC );

            if( m_ttx->CanTransmitFrom( qd ) )
            {
                NS_ASSERT( !m_ttx->IsNextFlitHead( qd ) );

                candidates.push_back( qd );
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
        m_legalQueue[ winner.outputVC ] = ANY_QUEUE;
    }
    else
    {
        if (m_ttx->IsNextFlitHead( winner ) )
        {
            // FIXME, print both inVC and outVC?
            NS_LOG_LOGIC( "vc=" << (uint32_t) winner.outputVC 
                << " assigned to src=" << winner.port );
        }
        // Remember mapping
        m_legalQueue[ winner.outputVC ] = winner;
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

    NS_LOG_LOGIC( "Winner is "
        << winner.port << ":"
        << (uint32_t) winner.outputVC );

    return winner;
}

TocinoQueueDescriptor
TocinoSimpleArbiter::GetVCOwner( const uint8_t vc ) const
{
    NS_ASSERT( vc < m_legalQueue.size() );

    return m_legalQueue[vc];
}

const TocinoQueueDescriptor TocinoSimpleArbiter::ANY_QUEUE = 
    TocinoQueueDescriptor( TOCINO_INVALID_PORT-1, 
            TOCINO_INVALID_VC-1, TOCINO_INVALID_VC-1 );
}
