/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_QUEUE_H__
#define __TOCINO_QUEUE_H__

#include <stdint.h>
#include <queue>

#include "ns3/queue.h"

namespace ns3 {

class Packet;

class TocinoQueue : public Queue
{
public:
  static TypeId GetTypeId();

  TocinoQueue();
  ~TocinoQueue();

  bool IsFull();

private:
  unsigned int m_max_depth; // max depth in packets

  std:queue<Ptr<Packet>> q;
 
  Ptr<Packet> DoDequeue();
  bool DoEnqueue(Ptr<Packet> p);
  Ptr<Packet> DoPeek();
};

}

#endif /* __TOCINO_QUEUE_H__ */

