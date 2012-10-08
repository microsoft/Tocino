/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

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
                   UintegerValue (MAXDEPTH),
                   MakeUintegerAccessor (&CallbackQueue::m_maxDepth),
                   MakeUintegerChecker<uint32_t>());
  return tid;
}

CallbackQueue::CallbackQueue() : Queue(), m_q ()
{
  int32_t i;
  for (i = 0; i < 2; i++)
    {
      m_cb[i].m_sense = EmptyEntries;
      m_cb[i].m_mark = 0;
      m_cb[i].m_cc = AtMark;
      //m_cb[i].m_cbo = 0;
      m_cb[i].m_cbState = OFF;
    }

  m_maxDepth = MAXDEPTH; // should be consistent with value in TypeId
  m_fullwm = 0;
  m_freewm = 0;
}

bool
CallbackQueue::IsFull() {return (m_q.size() == m_maxDepth);}

bool
CallbackQueue::IsEmpty() {return (m_q.size() == 0);}

bool
CallbackQueue::IsAlmostFull() {return ((m_maxDepth - m_q.size()) <= m_freewm);}

bool
CallbackQueue::IsAlmostEmpty() {return (m_q.size() <= m_fullwm);}

bool
CallbackQueue::EvalCallbackCondition(uint32_t i)
{
  bool c;
  uint32_t n; // value to test against

  if (i > 1) return false;

  if (m_cb[i].m_sense == FullEntries)
  {
      n = m_q.size(); // number of slots is number of full queue entries
  }
  else
  {
      n = m_maxDepth - m_q.size(); // number of slots is number of empty queue entries
  }

  c = (((m_cb[i].m_cc == FallingBelowMark) && (n < m_cb[i].m_mark)) ||
      ((m_cb[i].m_cc == AtMark) && (n == m_cb[i].m_mark)) ||
      ((m_cb[i].m_cc == RisingAboveMark) && (n > m_cb[i].m_mark)));
  return c;
}

bool
CallbackQueue::RegisterCallback(uint32_t i, Callback<void> cbo, uint32_t n, CallbackSense s, CallbackCondition cc)
{
    if (i > 1) return false; // only 2 callbacks per queue

    m_cb[i].m_sense = s;
    m_cb[i].m_mark = n;
    m_cb[i].m_cc = cc;
    m_cb[i].m_cbState = (EvalCallbackCondition(i))? SENT:READY;
    m_cb[i].m_cbo = cbo;
    return true;
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
      if (m_cb[i].m_cbState == OFF) continue;

      c = EvalCallbackCondition(i);
      if (c)
	{
	  if (m_cb[i].m_cbState == READY) // callback hasn't been sent yet
	    {
	      m_cb[i].m_cbState = SENT;
	      m_cb[i].m_cbo(); // invoke callback
	    }
	}
      else
	{
	  m_cb[i].m_cbState = READY; // condition is now false
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
