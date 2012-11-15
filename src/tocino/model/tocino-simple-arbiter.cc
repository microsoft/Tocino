/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-simple-arbiter.h"

#include "ns3/log.h"

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

uint32_t TocinoSimpleArbiter::Arbitrate()
{
    uint32_t winner = TOCINO_INVALID_QUEUE;

    if( m_currentWinner != TOCINO_INVALID_QUEUE )
    {
        if( m_ttx->IsQueueNotEmpty( m_currentWinner ) )
        {
            // Continue previous flow
            winner = m_currentWinner;
        }
    }
    else
    {
        // Pick a new flow
        // FIXME: obvious starvation concern
        for( uint32_t i = 0; i < m_tnd->GetNQueues(); i++ )
        {
            if( m_ttx->IsQueueNotEmpty( i ) )
            {
                winner = i;
                break;
            }
        }
      
        m_currentWinner = winner;
    }

    if( winner == TOCINO_INVALID_QUEUE )
    {
        return DO_NOTHING;
    }

    if( m_ttx->IsQueueNotEmpty( winner ) )
    {
        if( m_ttx->IsNextFlitTail( winner ) )
        {
            // Flow ending, reset
            m_currentWinner = TOCINO_INVALID_QUEUE;
        }
    }
    
    return winner;
}

}
