/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __CALLBACK_QUEUE_H__
#define __CALLBACK_QUEUE_H__

#include <queue>
#include "ns3/queue.h"

namespace ns3 {

class Packet;

class CallbackQueue : public Queue 
{
public:
  static TypeId GetTypeId(void);

  CallbackQueue();
  ~CallbackQueue() {};

  bool IsFull();
  bool IsAlmostFull();
  bool IsAlmostEmpty();
  bool IsEmpty();

  typedef void (*CBQCallback)(); // CallbackQueue callback
  enum CallbackCondition {FallingBelowMark, AtMark, RisingAboveMark};
  void RegisterCallback(uint32_t i, CBQCallback fptr, uint32_t mark, CallbackCondition cc);
  void DisableCallback(uint32_t i) {m_cbState[i] = OFF;}
  void SetHWM(uint32_t n) {m_hwm = n;} // set high water mark
  void SetLWM(uint32_t n) {m_lwm = n;} // set low water mark

private:
  bool DoEnqueue(Ptr<Packet> p);
  Ptr<Packet> DoDequeue(void);
  Ptr<const Packet> DoPeek(void) const;

  uint32_t m_mark[2];
  CallbackCondition m_cc[2];
  CBQCallback m_cb[2];
  enum CallbackState {OFF, READY, SENT};
  CallbackState m_cbState[2];
  void DoCallbacks();

  uint32_t m_maxDepth; // max depth of queue
  uint32_t m_hwm; // at and above this point is called almostfull
  uint32_t m_lwm; // at and below this point is called almostempty
  std::queue<Ptr<Packet> > m_q;
};

} // namespace ns3

#endif /* __CALLBACK_QUEUE_H__ */

