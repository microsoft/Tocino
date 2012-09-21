/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-queue.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TocinoQueue);

TypeId TocinoQueue::GetTypeId(void)
{
  static TypeId tid = TypeId( "ns3::TocinoQueue" )
    .SetParent<Queue>()
    .AddConstructor<TocinoQueue>()
    .AddAttribute ("Depth", "Maximum queue depth.",
                   unsigned int (0));
  return tid;
}

TocinoQueue::TocinoQueue()
{
}

TocinoQueue::~TocinoQueue()
{
}

bool
TocinoQueue::IsFull() {return (q.size() == m_max_depth);}

bool
TocinoQueue::DoEnqueue(Ptr<Packet> p) {q.push(p);}

<Ptr>Packet
TocinoQueue::DoDequeue() {return q.pop();}

<Ptr>Packet
TocinoQueue::DoPeek() {return q.front();}
