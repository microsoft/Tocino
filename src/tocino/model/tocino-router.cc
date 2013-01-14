/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-router.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TocinoRouter");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (TocinoRouter);

TypeId TocinoRouter::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoRouter" )
        .SetParent<Object>();
    return tid;
}

// N.B. This is intentionally distinct from TOCINO_INVALID_QUEUE
const TocinoQueueDescriptor TocinoRouter::CANNOT_ROUTE = 
    TocinoQueueDescriptor( TOCINO_INVALID_PORT-1, TOCINO_INVALID_VC-1 );

}
