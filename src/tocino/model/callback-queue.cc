/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

#include "callback-queue.h"

NS_LOG_COMPONENT_DEFINE ("CallbackQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CallbackQueue);

TypeId CallbackQueue::GetTypeId(void)
{
  static TypeId tid = TypeId( "ns3::CallbackQueue" )
    .SetParent<Queue>()
    .AddConstructor<CallbackQueue>()
    .AddAttribute ("Depth", "Maximum queue depth.",
                   UintegerValue (8),
                   MakeUintegerAccessor (&CallbackQueue::m_maxDepth),
                   MakeUintegerChecker<uint32_t>());
  return tid;
}

CallbackQueue::CallbackQueue() : Queue(), m_q ()
{
  int32_t i;
  for (i = 0; i < 2; i++)
    {
      m_mark[i] = 0; m_cc[i] = AtMark; m_cb[i] = NULL; m_cbState[i] = OFF;
    }

  m_hwm = m_maxDepth;
  m_lwm = 0;
}

bool
CallbackQueue::IsFull() {return (m_q.size() == m_maxDepth);}

bool
CallbackQueue::IsEmpty() {return (m_q.size() == 0);}

bool
CallbackQueue::IsAlmostFull() {return (m_q.size() >= m_hwm);}

bool
CallbackQueue::IsAlmostEmpty() {return (m_q.size() <= m_lwm);}

void
CallbackQueue::RegisterCallback(uint32_t i, CBQCallback fptr, uint32_t mark, CallbackCondition cc)
{
  m_cb[i] = fptr;
  m_mark[i] = mark;
  m_cc[i] = cc;
  m_cbState[i] = READY;
}

void
CallbackQueue::DoCallbacks()
{
  // callbacks fire if they are in the READY state and the condition is true
  // once fired, the state becomes SENT (and the callback will not fire again)
  // one the callback condition becomes false the callback state returns to READY
  // and the callback is eligible to fire again - this models edge-triggered behavior
  uint32_t i;
  bool c;
  for (i = 0; i < 2; i++)
    {
      if (m_cbState[i] == OFF) continue;

      c = ((m_cc[i] == FallingBelowMark) && (m_mark[i] < m_q.size())) ||
	((m_cc[i] == AtMark) && (m_mark[i] == m_q.size())) ||
	((m_cc[i] == RisingAboveMark) && (m_mark[i] > m_q.size()));

      if (c)
	{
	  if (m_cbState[i] == READY) // callback hasn't been sent yet
	    {
	      m_cbState[i] = SENT;
	      m_cb[i](); // invoke callback
	    }
	}
      else
	{
	  m_cbState[i] = READY; // condition is now false
	}
    }
}
	  
bool
CallbackQueue::DoEnqueue(Ptr<Packet> p) 
{
  if (m_q.size() >= m_maxDepth) return false;

  m_q.push (p);
  DoCallbacks();
  return true;
}

Ptr<Packet>
CallbackQueue::DoDequeue() 
{
  if (m_q.empty ()) return 0;

  Ptr<Packet> p = m_q.front ();
  m_q.pop ();
  DoCallbacks();
  return p;
}

Ptr<const Packet>
CallbackQueue::DoPeek() const
{
  if (m_q.empty ()) return 0;

  Ptr<const Packet> p = m_q.front ();
  return p;
}

} // namespace ns3
