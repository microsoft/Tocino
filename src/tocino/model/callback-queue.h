/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __CALLBACK_QUEUE_H__
#define __CALLBACK_QUEUE_H__

#include <deque>
#include "ns3/queue.h"
#include "ns3/packet.h"
#include "ns3/callback.h"

namespace ns3 {

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
    enum CallbackSense {FullEntries, EmptyEntries};
    bool RegisterCallback(uint32_t i, Callback<void> cbo, uint32_t n, CallbackSense s, CallbackCondition cc);
    void DisableCallback(uint32_t i) {m_cb[i].m_cbState = OFF;}
    void SetFreeWM(uint32_t n) {m_freewm = (n>m_maxDepth)? 0:n;} // set high water mark
    void SetFullWM(uint32_t n) {m_fullwm = (n>m_maxDepth)? 0:n;} // set low water mark
    void SetName(char *name) {strncpy(m_name, name, 31); m_name[31] = 0;}
    uint32_t Size() {return m_q.size();}
    Ptr<Packet> At(uint32_t i) {return m_q.at(i);}
    
private:
    static const uint32_t DEFAULT_MAXDEPTH = 8;
    static const uint32_t DEFAULT_FREEWM = 0;

    char m_name[32]; // symbolic name for queue - debug assist

    bool DoEnqueue(Ptr<Packet> p);
    Ptr<Packet> DoDequeue(void);
    Ptr<const Packet> DoPeek(void) const;
    
    enum CallbackState {OFF, READY, SENT};
    struct {
        uint32_t m_sense; // full queue slots (1) or empty ones (0)
        uint32_t m_mark; // how many slots
        CallbackCondition m_cc; // describe transition
        CallbackState m_cbState; // current state of callback trigger
        Callback<void> m_cbo; // callback object
    } m_cb[2];

    bool EvalCallbackCondition(uint32_t i); // evaluate condition triggering callback i
    void DoCallbacks(); // generate callbacks if triggered
    
    uint32_t m_maxDepth; // max depth of queue
    uint32_t m_freewm; // free queue entries less than or equal to this is called almostfull
    uint32_t m_fullwm; // full queue entries at and below this point is called almostempty
    std::deque<Ptr<Packet> > m_q;
};

} // namespace ns3

#endif /* __CALLBACK_QUEUE_H__ */

