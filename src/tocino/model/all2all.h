/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __A2A_H_
#define __A2A_H_

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/uinteger.h"

#include "tocino-address.h"
#include "tocino-net-device.h"

namespace ns3 {

class All2All : public Application
{
public:
    static TypeId GetTypeId (void);
    All2All();
    virtual ~All2All();

    void AddRemote(Ptr<NetDevice> nd);
    void ScheduleTransmit(Time dt);
    void Send(void);
    void Receive();

protected:
    virtual void DoDispose(void);

private:
    virtual void StartApplication(void); //called at t==Start
    virtual void StopApplication(void); // called at t==Stop

    std::vector<Ptr<NetDevice> > m_netDevices; 
    Ptr<NetDevice> m_myNetDevice;

    // describe workload
    Time m_mtbs; // mean time between sends
    Time m_maxdt; // max time between sends
    uint32_t m_size; // size of send
    EventId m_sendEvent;

    Ptr<UniformRandomVariable> m_destRV;
    Ptr<ExponentialRandomVariable> m_dtRV;
};

} // namespace ns3

#endif // __A2A_H_
