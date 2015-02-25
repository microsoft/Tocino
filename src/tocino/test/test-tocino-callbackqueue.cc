/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <vector>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

#include "ns3/callback-queue.h"
#include "ns3/test.h"

#include "test-tocino-callbackqueue.h"

using namespace ns3;

uint32_t testBecameAlmostEmptyCB = 0;
uint32_t testBecameNotFullCB = 0;

void
BecameAlmostEmptyCB()
{
  testBecameAlmostEmptyCB += 1;
}

void
BecameNotFullCB()
{
  testBecameNotFullCB += 1;
}

TestTocinoCallbackQueue::TestTocinoCallbackQueue()
  : TestCase ("Validate CallbackQueue functionality")
{
}

TestTocinoCallbackQueue::~TestTocinoCallbackQueue() {}


// macro to simplify testing queue state
#define CBQStatus (					\
		   (((q->IsFull())? 8:0)) |		\
		   (((q->IsAlmostFull())? 4:0)) |	\
		   (((q->IsAlmostEmpty())? 2:0)) |	\
		   (((q->IsEmpty())? 1:0)))
  
void
TestTocinoCallbackQueue::DoRun (void)
{
  Config::SetDefault("ns3::CallbackQueue::Depth", UintegerValue(4));

  Ptr<CallbackQueue> q = CreateObject<CallbackQueue>();
  Ptr<Packet> p0 = Create<Packet>( 64 );
  Ptr<Packet> p1 = Create<Packet>( 64 );
  Ptr<Packet> p2 = Create<Packet>( 64 );
  Ptr<Packet> p3 = Create<Packet>( 64 );
  Ptr<Packet> p4 = Create<Packet>( 64 );
  Ptr<Packet> t;


  // validate IsEmpty()
  NS_TEST_ASSERT_MSG_EQ(q->Size(), 0, "queue size");
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 3, "failed initial state test");

  // push 4 items and validate IsFull()
  q->Enqueue(p0);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed push 1");
  q->Enqueue(p1);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed push 2");
  q->Enqueue(p2);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed push 3");
  q->Enqueue(p3);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 12, "failed push 4");

  // pop 4 items and validate IsEmpty()
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed pop 1");
  NS_TEST_ASSERT_MSG_EQ(t, p0, "bad data from pop 1");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed pop 2");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed pop 3");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 3, "failed pop 4");
  
  // set HWM and LWM - validate IsAlmostFull() and IsAlmostEmpty()
  q->SetFreeWM(1);
  q->SetFullWM(1);

  q->Enqueue(p0);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 2, "failed push 5");
  q->Enqueue(p1);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed push 6");
  q->Enqueue(p2);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 4, "failed push 7");
  q->Enqueue(p3);
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 12, "failed push 8");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 4, "failed pop 5");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 0, "failed pop 6");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 2, "failed pop 7");
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(CBQStatus, 3, "failed pop 8");

  // configure callbacks
  // callback 0 "BecomeAlmostEmptyCB" should fire when queue size falls below 2
  // callback 1 "BecomeNotFullCB" should fire on transition from Full to not Full
  // queue size at this point in the test should be 0
  Callback<void> cb0, cb1;
  cb0 = MakeCallback(BecameAlmostEmptyCB);
  cb1 = MakeCallback(BecameNotFullCB);

  NS_TEST_ASSERT_MSG_EQ(q->IsEmpty(), true, "failed initial state test");
  q->RegisterCallback(0, cb0, 2, CallbackQueue::FullEntries, CallbackQueue::FallingBelowMark);
  q->RegisterCallback(1, cb1, 0, CallbackQueue::EmptyEntries, CallbackQueue::RisingAboveMark);

  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 0, "spurious call to BecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 0, "spurious call to testBecameNotFullCB");

  // validate callbacks
  q->Enqueue(p0); // queue size == 1
  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 0, "spurious call to BecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 0, "spurious call to testBecameNotFullCB");
  q->Enqueue(p1); // == 2
  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 0, "spurious call to BecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 0, "spurious call to testBecameNotFullCB");
  q->Enqueue(p2); // == 3
  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 0, "spurious call to BecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 0, "spurious call to testBecameNotFullCB");
  q->Enqueue(p3); // == 4; now full
  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 0, "spurious call to BecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 0, "spurious call to testBecameNotFullCB");

  // BecameNotFullCB should be invoked after next pop
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 0, "spurious call to BecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 1, "bad state on testBecameNotFullCB");

  t = q->Dequeue();

  // BecameAlmostEmpty should be invoked after next pop +1000
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(testBecameAlmostEmptyCB, 1, "bad state on testBecameAlmostEmptyCB");
  NS_TEST_ASSERT_MSG_EQ(testBecameNotFullCB, 1, "bad state on testBecameNotFullCB");
}
