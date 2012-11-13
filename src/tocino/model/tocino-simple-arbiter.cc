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
{}

void TocinoSimpleArbiter::Initialize( Ptr<TocinoNetDevice> tnd, const TocinoTx* ttx )
{
    m_tnd = tnd;
    m_ttx = ttx;
}

int TocinoSimpleArbiter::Arbitrate()
{
    const int TOTAL_QUEUES = m_tnd->GetNPorts() * m_tnd->GetNVCs();

    // trivial arbitration - obvious starvation concern
    for ( int i = 0; i < TOTAL_QUEUES; i++)
    {
        if( m_ttx->IsQueueNotEmpty( i ) ) return i;
    }

    return TOCINO_INVALID_QUEUE;
}

}
