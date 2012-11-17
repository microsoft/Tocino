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
{}

void TocinoSimpleArbiter::Initialize( Ptr<TocinoNetDevice> tnd, const TocinoTx* ttx )
{
    m_tnd = tnd;
    m_ttx = ttx;
    m_legalPort.assign( m_tnd->GetNVCs(), ANY_PORT );
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

TocinoSimpleArbiter::QueueVector
TocinoSimpleArbiter::BuildCandidateSet() const
{
    NS_ASSERT( m_tnd->GetNVCs() == m_legalPort.size() );

    std::vector<uint32_t> candidates;

    for( uint8_t vc = 0; vc < m_tnd->GetNVCs(); ++vc )
    {
        if( m_legalPort[vc] == ANY_PORT )
        {
            // can select any port that is ready
            for( uint32_t port = 0; port < m_tnd->GetNPorts(); ++port )
            {
                uint32_t queue = m_tnd->PortToQueue( port, vc );

                if( m_ttx->IsQueueNotEmpty( queue ) )
                {
                    candidates.push_back( queue );
                }
            }
        }
        else
        {
            // can only select the allocated port, if ready
            uint32_t queue = m_tnd->PortToQueue( m_legalPort[vc], vc );

            if( m_ttx->IsQueueNotEmpty( queue ) )
            {
                candidates.push_back( queue );
            }
        }
    }

    return candidates;
}

uint32_t
TocinoSimpleArbiter::FairSelectWinner( const QueueVector& cand ) const
{
    UniformVariable rv;
    return cand[ rv.GetInteger( 0, cand.size()-1 ) ];
}

void
TocinoSimpleArbiter::UpdateState( uint32_t winner )
{
    if( m_ttx->IsNextFlitTail( winner ) )
    {
        // Flow ending, reset
        m_legalPort[ m_tnd->QueueToVC(winner) ] = ANY_PORT;
    }
    else
    {
        // Remember mapping
        m_legalPort[ m_tnd->QueueToVC(winner) ] = m_tnd->QueueToPort(winner); 
    }
}

uint32_t
TocinoSimpleArbiter::Arbitrate()
{
    QueueVector candidates = BuildCandidateSet();

    if( candidates.empty() )
    {
        return DO_NOTHING;
    }

    uint32_t winner = FairSelectWinner( candidates );

    UpdateState( winner );

    return winner;
}

const uint32_t TocinoSimpleArbiter::ANY_PORT = TOCINO_INVALID_PORT-1;

}
