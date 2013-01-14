/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-simple-arbiter.h"

#include <vector>

#include "ns3/log.h"
#include "ns3/random-variable.h"

#include "tocino-misc.h"
#include "tocino-tx.h"
#include "tocino-flit-id-tag.h"

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
            for( uint32_t inPort = 0; inPort < m_tnd->GetNPorts(); ++inPort )
            {
                TocinoQueueDescriptor qd( inPort, outVC );

                if( m_ttx->CanTransmitFrom( qd ) )
                {
                    NS_ASSERT( m_ttx->IsNextFlitHead( qd ) );

                    candidates.push_back( qd );
                }
            }
        }
        else
        {
            // can only select the allocated queue, if ready
            TocinoQueueDescriptor qd = m_legalQueue[outVC];

            NS_ASSERT( m_legalQueue[outVC].GetVirtualChannel() == outVC );

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
    // try to select a different outVC than last time
    TocinoQueueDecriptor winner;
    QueueVector newCand;
   
    if( m_lastVC != TOCINO_INVALID_VC )
    {
        for( unsigned i = 0; i < cand.size(); ++i )
        {
            uint8_t outVC = cand[i].outVC;

            if( outVC != m_lastVC )
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
    
    m_lastVC = winner.outVC;
    return winner;
#endif 
}

void
TocinoSimpleArbiter::UpdateState( const TocinoQueueDescriptor winner )
{
    const uint32_t outputVC = winner.GetVirtualChannel();
    const uint32_t inputPort = winner.GetPort();

    if( m_ttx->IsNextFlitTail( winner ) )
    {
        // Flow ending, reset
        m_legalQueue[ outputVC ] = ANY_QUEUE;
    }
    else
    {
        if (m_ttx->IsNextFlitHead( winner ) )
        {
            NS_LOG_LOGIC( "outputVC="
                    << outputVC
                    << " assigned to inputPort="
                    << inputPort );
        }
        // Remember mapping
        m_legalQueue[ outputVC ] = winner;
    }
}

TocinoQueueDescriptor
TocinoSimpleArbiter::Arbitrate()
{
    QueueVector candidates = BuildCandidateSet();

    if( candidates.empty() )
    {
        NS_LOG_LOGIC( "no candidates" );
        return DO_NOTHING;
    }

    TocinoQueueDescriptor winner = FairSelectWinner( candidates );

    UpdateState( winner );
    
    const uint32_t outputVC = winner.GetVirtualChannel();
    const uint32_t inputPort = winner.GetPort();

    NS_LOG_LOGIC( "winner is inputPort=" << inputPort 
            << ", outputVC=" << outputVC );
            
    return winner;
}

TocinoQueueDescriptor
TocinoSimpleArbiter::GetVCOwner( const uint32_t outVC ) const
{
    NS_ASSERT( outVC < m_legalQueue.size() );

    return m_legalQueue[outVC];
}

const TocinoQueueDescriptor TocinoSimpleArbiter::ANY_QUEUE = 
    TocinoQueueDescriptor( TOCINO_INVALID_PORT-1, TOCINO_INVALID_VC-1 );
}
