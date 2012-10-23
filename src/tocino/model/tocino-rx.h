/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_RX_H__
#define __TOCINO_RX_H__

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"

namespace ns3
{

class NetDevice;
class TocinoNetDevice;
class Packet;
class CallbackQueue;

class TocinoRx
{
public:
    TocinoRx( uint32_t nPorts, uint32_t nVCs);
    ~TocinoRx();
    
    Ptr<NetDevice> GetNetDevice();
    
    void Receive(Ptr<const Packet> p);
    
    friend class TocinoNetDevice;
    friend class TocinoTx;
private:
    
    static const uint32_t INVALID_PORT = -1;

    uint32_t m_portNumber;
    uint32_t m_currentRoutePort;

    Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice
    
    std::vector< Ptr <CallbackQueue> > m_queues; // packet queues to write
    
    
    bool IsBlocked();
    void CheckForUnblock(); // called from TocinNetDeviceTransmitter
    
    uint32_t Route(Ptr<const Packet> p); // TODO: make this runtime settable
};

} // namespace ns3

#endif // __TOCINO_RX_H__
