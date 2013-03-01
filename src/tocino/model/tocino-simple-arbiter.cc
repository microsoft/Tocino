/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-simple-arbiter.h"

#include <vector>

#include "ns3/log.h"
#include "ns3/random-variable.h"
#include "ns3/boolean.h"

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
        .AddAttribute( "InterleaveVCs", 
            "Bias arbitraton to promote interleaving of VCs on the wire.",
            BooleanValue( false ),
            MakeBooleanAccessor( &TocinoSimpleArbiter::m_interleaveVCs),
            MakeBooleanChecker() )
        .AddConstructor<TocinoSimpleArbiter>();
    return tid;
}
   
TocinoSimpleArbiter::TocinoSimpleArbiter()
    : m_tnd( NULL )
    , m_ttx( NULL )
    , m_interleaveVCs( false )
    , m_lastVC( TOCINO_INVALID_VC )
{}

void TocinoSimpleArbiter::Initialize( const TocinoNetDevice* tnd, const TocinoTx* ttx )
{
    m_tnd = tnd;
    m_ttx = ttx;
    m_legalQueue.vec.assign( m_tnd->GetNVCs(), ANY_QUEUE );
}

TocinoSimpleArbiter::AllocVector
TocinoSimpleArbiter::BuildCandidateSet() const
{
    AllocVector candidates;

    for( TocinoOutputVC outputVC = 0; outputVC < m_tnd->GetNVCs(); ++outputVC )
    {
        if( GetVCOwner( outputVC ) == ANY_QUEUE )
        {
            // can select any queue that is ready
            for( TocinoInputPort inputPort = 0; 
                    inputPort < m_tnd->GetNPorts(); ++inputPort )
            {
                if( m_ttx->CanTransmitFrom( inputPort, outputVC ) )
                {
                    NS_ASSERT( m_ttx->IsNextFlitHead( inputPort, outputVC ) );
            
                    TocinoArbiterAllocation alloc( inputPort, outputVC );
                    candidates.push_back( alloc );
                }
            }
        }
        else
        {
            // can only select the allocated queue, if ready
            TocinoArbiterAllocation alloc = GetVCOwner( outputVC );

            NS_ASSERT( GetVCOwner( outputVC ).outputVC == outputVC );

            if( m_ttx->CanTransmitFrom( alloc.inputPort, alloc.outputVC ) )
            {
                NS_ASSERT( !m_ttx->IsNextFlitHead( alloc.inputPort, alloc.outputVC ) );

                candidates.push_back( alloc );
            }
        }
    }

    return candidates;
}

TocinoArbiterAllocation
TocinoSimpleArbiter::SelectWinnerInterleaveVCs( const AllocVector& cand ) const
{
    UniformVariable rv;

    // try to select a different outVC than last time
    TocinoArbiterAllocation winner( DO_NOTHING );
    AllocVector newCand;

    if( m_lastVC != TOCINO_INVALID_VC )
    {
        for( unsigned i = 0; i < cand.size(); ++i )
        {
            TocinoOutputVC outputVC = cand[i].outputVC;

            if( outputVC != m_lastVC )
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

    m_lastVC = winner.outputVC;

    return winner;
}

TocinoArbiterAllocation
TocinoSimpleArbiter::FairSelectWinner( const AllocVector& cand ) const
{
    UniformVariable rv;
        
    return cand[ rv.GetInteger( 0, cand.size()-1 ) ];
}

void
TocinoSimpleArbiter::UpdateState( const TocinoArbiterAllocation winner )
{
    const uint32_t inputPort = winner.inputPort.AsUInt32();
    const uint32_t outputVC = winner.outputVC.AsUInt32();

    if( m_ttx->IsNextFlitTail( inputPort, outputVC ) )
    {
        // Flow ending, reset
        m_legalQueue.vec[ outputVC ] = ANY_QUEUE;
    }
    else
    {
        // Remember mapping
        m_legalQueue.vec[ outputVC ] = winner;

        if (m_ttx->IsNextFlitHead( inputPort, outputVC ) )
        {
            NS_LOG_LOGIC( "outputVC=" << outputVC
                    << " allocated to inputPort=" << inputPort );
        }
    }
}

TocinoArbiterAllocation
TocinoSimpleArbiter::Arbitrate()
{
    AllocVector candidates = BuildCandidateSet();

    if( candidates.empty() )
    {
        NS_LOG_LOGIC( "no candidates" );
        return DO_NOTHING;
    }

    TocinoArbiterAllocation winner;

    if( m_interleaveVCs )
    {
        winner = SelectWinnerInterleaveVCs( candidates );
    }
    else
    {
        winner = FairSelectWinner( candidates );
    }

    UpdateState( winner );
    
    NS_LOG_LOGIC( "winner is inputPort=" << winner.inputPort 
            << ", outputVC=" << winner.outputVC );
            
    return winner;
}

TocinoArbiterAllocation
TocinoSimpleArbiter::GetVCOwner( const TocinoOutputVC outputVC ) const
{
    NS_ASSERT( outputVC < m_legalQueue.vec.size() );

    return m_legalQueue.vec[ outputVC.AsUInt32() ];
}

const TocinoArbiterAllocation TocinoSimpleArbiter::ANY_QUEUE = 
    TocinoArbiterAllocation( TOCINO_INVALID_PORT, TOCINO_INVALID_VC );
}
