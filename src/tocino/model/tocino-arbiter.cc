/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#include "tocino-arbiter.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TocinoArbiter");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (TocinoArbiter);

TypeId TocinoArbiter::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoArbiter" )
        .SetParent<Object>();
    return tid;
}

}
