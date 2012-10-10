/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <vector>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/test.h"
#include "ns3/tocino-net-device.h"
#include "ns3/tocino-flit-header.h"

#include "tocino-flitloopback.h"

using namespace ns3;

TocinoFlitLoopback::TocinoFlitLoopback()
  : TestCase ("Transmit and receive 1 flit on loopback")
{
}

TocinoFlitLoopback::~TocinoFlitLoopback() {}

namespace
{
    unsigned totalCount;
    unsigned totalBytes;

    bool AcceptPacket( Ptr<NetDevice>, Ptr<const Packet> p, uint16_t, const Address& )
    {
        totalCount++;
        totalBytes += p->GetSize();
        
        return true;
    }
}

void TocinoFlitLoopback::TestHelper( const unsigned COUNT, const unsigned BYTES )
{
    Ptr<Packet> p = Create<Packet>( BYTES );
   
    Ptr<TocinoNetDevice> tnd = CreateObject<TocinoNetDevice>();
    
    tnd->Initialize();
    tnd->SetReceiveCallback( MakeCallback( AcceptPacket ) );
    
    totalCount = 0;
    totalBytes = 0;

    for( unsigned i = 0; i < COUNT; ++i )
    {
        tnd->Send( p, TocinoAddress(), 0 );
    }

    NS_TEST_ASSERT_MSG_EQ( totalCount, COUNT, "Got unexpected total packet count" );
    NS_TEST_ASSERT_MSG_EQ( totalBytes, BYTES*COUNT, "Got unexpected total packet bytes" );
}

void
TocinoFlitLoopback::DoRun (void)
{
    LogComponentEnable("TocinoNetDevice", LOG_LEVEL_ALL);
    LogComponentEnable("TocinoTx", LOG_LEVEL_ALL);
    //LogComponentEnableAll(LOG_LEVEL_ALL);

    TestHelper( 1, 20 );
    TestHelper( 1, 123 );
    TestHelper( 2, 32 );
}
