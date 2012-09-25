/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_QUEUE_H__
#define __TOCINO_QUEUE_H__

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"

namespace ns3 {

class Packet;

class TocinoQueue : public Queue 
{
public:
  static TypeId GetTypeId(void);

  TocinoQueue();
  ~TocinoQueue();

  bool IsFull();

private:
  bool DoEnqueue(Ptr<Packet> p);
  Ptr<Packet> DoDequeue(void);
  Ptr<const Packet> DoPeek(void) const;

  uint32_t m_maxDepth; // max depth in packets
  std::queue<Ptr<Packet> > m_packets;
};

} // namespace ns3

#endif /* __TOCINO_QUEUE_H__ */

