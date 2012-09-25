/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include "tocino-queue.h"

NS_LOG_COMPONENT_DEFINE ("TocinoQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TocinoQueue);

TypeId TocinoQueue::GetTypeId(void)
{
  static TypeId tid = TypeId( "ns3::TocinoQueue" )
    .SetParent<Queue>()
    .AddConstructor<TocinoQueue>()
    .AddAttribute ("Depth", "Maximum queue depth.",
                   UintegerValue (8),
                   MakeUintegerAccessor (&TocinoQueue::m_maxDepth),
                   MakeUintegerChecker<uint32_t>());
  return tid;
}

TocinoQueue::TocinoQueue() : Queue(), m_packets ()
{
}

TocinoQueue::~TocinoQueue()
{
}

bool
TocinoQueue::IsFull() {return (m_packets.size() == m_maxDepth);}

bool
TocinoQueue::DoEnqueue(Ptr<Packet> p) 
{
  if (m_packets.size () >= m_maxDepth)
  {
    NS_LOG_ERROR ("Push on full queue");
    return false;
  } 
  m_packets.push (p);

  return true;
}

Ptr<Packet>
TocinoQueue::DoDequeue() 
{
  if (m_packets.empty ())
    {
      NS_LOG_ERROR ("Pop on empty queue");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
  m_packets.pop ();
 
 return p;
}

Ptr<const Packet>
TocinoQueue::DoPeek() const
{
  if (m_packets.empty ())
    {
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
  return p;
}

} // namespace ns3
