/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
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
                   MakeUintegerChecker<uint32_t> ());
  return tid;
}

TocinoQueue::TocinoQueue() : Queue(), m_packets ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TocinoQueue::~TocinoQueue()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
TocinoQueue::IsFull() {return (m_packets.size() == m_maxDepth);}

bool
TocinoQueue::DoEnqueue(Ptr<Packet> p) 
{ 
  NS_LOG_FUNCTION (this << p);

  if (m_packets.size () >= m_maxPackets)
  {
    NS_LOG_LOGIC ("Error: queue full");
    return false;
  } 
  m_packets.push (p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  return true;
}

<Ptr>Packet
TocinoQueue::DoDequeue() 
{
  NS_LOG_FUNCTION (this);
 
  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Error: queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
  m_packets.pop ();
 
  NS_LOG_LOGIC ("Popped " << p);
  NS_LOG_LOGIC ("Number packets " << m_packets.size ());

  return p;
}

<Ptr>Packet
TocinoQueue::DoPeek()
{
  NS_LOG_FUNCTION (this);
 
  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
 
  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  return p;
}

} // namespace ns3
