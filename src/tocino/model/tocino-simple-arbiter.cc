/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-simple-arbiter.h"

#include <vector>

#include "ns3/log.h"
#include "ns3/random-variable.h"

#include "tocino-misc.h"
#include "tocino-tx.h"

NS_LOG_COMPONENT_DEFINE ("TocinoSimpleArbiter");

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
    , m_currentWinner( TOCINO_INVALID_QUEUE )
{}

void TocinoSimpleArbiter::Initialize( Ptr<TocinoNetDevice> tnd, const TocinoTx* ttx )
{
    m_tnd = tnd;
    m_ttx = ttx;
}

bool TocinoSimpleArbiter::AllQueuesEmpty() const
{
    for( uint32_t i = 0; i < m_tnd->GetNQueues(); i++ )
    {
        if( m_ttx->IsQueueNotEmpty( i ) )
        {
            return false;
        }
    }

    return true;
}

uint32_t TocinoSimpleArbiter::FairSelectWinner() const
{
    std::vector<uint32_t> ready;

    for( uint32_t i = 0; i < m_tnd->GetNQueues(); i++ )
    {
        if( m_ttx->IsQueueNotEmpty( i ) )
        {
            ready.push_back(i);
        }
    }

    UniformVariable rv;
    
    return ready[ rv.GetInteger( 0, ready.size()-1 ) ];
}

uint32_t TocinoSimpleArbiter::Arbitrate()
{
    uint32_t winner = TOCINO_INVALID_QUEUE;

    if( m_currentWinner != TOCINO_INVALID_QUEUE )
    {
        if( m_ttx->IsQueueEmpty( m_currentWinner ) )
        {
            return DO_NOTHING;
        }
        
        // Continue previous flow
        winner = m_currentWinner;
    }
    else
    {
        if( AllQueuesEmpty() )
        {
            return DO_NOTHING;
        }
        
        // Pick a new flow
        winner = FairSelectWinner();

        m_currentWinner = winner;
    }

    NS_ASSERT( winner != TOCINO_INVALID_QUEUE );
    NS_ASSERT( m_ttx->IsQueueNotEmpty( winner ) );

    if( m_ttx->IsNextFlitTail( winner ) )
    {
        // Flow ending, reset
        m_currentWinner = TOCINO_INVALID_QUEUE;
    }
    
    return winner;
}

}
