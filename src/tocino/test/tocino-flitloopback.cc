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

void
TocinoFlitLoopback::DoRun (void)
{
    //LogComponentEnableAll(LOG_LEVEL_ALL);
    LogComponentEnable("TocinoNetDevice", LOG_LEVEL_ALL);
    LogComponentEnable("TocinoTx", LOG_LEVEL_ALL);

    // create the net device
    Ptr<TocinoNetDevice> tnd = CreateObject<TocinoNetDevice>();
    tnd->Initialize();

    // define the header
    TocinoFlitHeader* fh = new TocinoFlitHeader();
    fh->SetSource(0);
    fh->SetDestination(0);
    fh->SetHead();
    fh->SetTail();
    fh->SetVirtualChannel(0);
    fh->SetLength(20);
    fh->SetType(TocinoFlitHeader::LLC);

    // create a payload
    Ptr<Packet> flit = Ptr<Packet>(new Packet(20));
  
    // wrap the header around the payload and inject
    flit->AddHeader(*fh);
    bool rc = tnd->InjectFlit(flit);
    NS_TEST_ASSERT_MSG_EQ(rc, true, "Injection failed");
 
    // look to see if it was ejected
    NS_TEST_ASSERT_MSG_EQ(tnd->GetEjectedFlitCount(), 1, "Bad ejected flit count");
}
