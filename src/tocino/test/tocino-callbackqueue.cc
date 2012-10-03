/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <vector>

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

// Include a header file from your module to test.
#include "ns3/callback-queue.h"

#include "ns3/test.h"

using namespace ns3;

uint32_t teststatus = 0;

void
BecameAlmostEmptyCB()
{
  teststatus += 1000;
}

void
BecameNotFullCB()
{
  teststatus += 1;
}

#include "tocino-callbackqueue.h"

TocinoCallbackQueue::TocinoCallbackQueue()
  : TestCase ("Validate CallbackQueue functionality")
{
}

TocinoCallbackQueue::~TocinoCallbackQueue() {}


// macro to simplify testing queue state
#define CBQStatus (					\
		   (((q->IsFull())? 8:0)) |		\
		   (((q->IsAlmostFull())? 4:0)) |	\
		   (((q->IsAlmostEmpty())? 2:0)) |	\
		   (((q->IsEmpty())? 1:0)))
  
void
TocinoCallbackQueue::DoRun (void)
{
  Config::SetDefault("ns3::CallbackQueue::Depth", UintegerValue(4));

  Ptr<CallbackQueue> q = CreateObject<CallbackQueue>();
  Ptr<Packet> p0 = new Packet(64);
  Ptr<Packet> p1 = new Packet(64);
  Ptr<Packet> p2 = new Packet(64);
  Ptr<Packet> p3 = new Packet(64);
  Ptr<Packet> p4 = new Packet(64);
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
  // callback 1 "BecomeNotFullCB" should fire when queue size falls below 4
  // queue size at this point in the test should be 0
  NS_TEST_ASSERT_MSG_EQ(q->IsEmpty(), true, "failed initial state test");
  q->RegisterCallback(0, BecameAlmostEmptyCB, 2, CallbackQueue::FallingBelowMark); // +1000
  q->RegisterCallback(1, BecameNotFullCB, 4, CallbackQueue::FallingBelowMark); // +1

  NS_TEST_ASSERT_MSG_EQ(teststatus, 0, "spurious call to callback 1");
  // validate callbacks
  q->Enqueue(p0); // queue size == 1
  NS_TEST_ASSERT_MSG_EQ(teststatus, 0, "spurious call to callback 2");
  q->Enqueue(p1); // == 2
  NS_TEST_ASSERT_MSG_EQ(teststatus, 0, "spurious call to callback 3");
  q->Enqueue(p2); // == 3
  NS_TEST_ASSERT_MSG_EQ(teststatus, 0, "spurious call to callback 4");
  q->Enqueue(p3); // == 4; now full
  NS_TEST_ASSERT_MSG_EQ(teststatus, 0, "spurious call to callback 5");

  // BecameNotFullCB should be invoked after next pop +1
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(teststatus, 1, "test status 1 failed");

  t = q->Dequeue();

  // BecameAlmostEmpty should be invoked after next pop +1000
  t = q->Dequeue();
  NS_TEST_ASSERT_MSG_EQ(teststatus, 1001, "test status 1001 failed");
}
