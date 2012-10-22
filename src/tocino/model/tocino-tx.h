/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TX_H__
#define __TOCINO_TX_H__

#include <stdint.h>

#include "ns3/ptr.h"
#include "ns3/packet.h"

#include "tocino-misc.h"

namespace ns3
{
   
class NetDevice;
class TocinoChannel;
class TocinoNetDevice;
class CallbackQueue;

class TocinoTx
{
public:
    TocinoTx( uint32_t nPorts, uint32_t nVCs );
    ~TocinoTx();
   
    void SetXState(TocinoFlowControlState s);
    TocinoFlowControlState GetXState();
    
    void SetChannel(Ptr<TocinoChannel> channel);
    void SendXOFF();
    void SendXON();
    
    Ptr<NetDevice> GetNetDevice();
    
    void Transmit();
    
    friend class TocinoNetDevice;
    
private:
    uint32_t m_portNumber;
    
    TocinoFlowControlState m_xstate;
    Ptr<Packet> m_packet;

    enum TocinoTransmitterState {IDLE, BUSY};
    TocinoTransmitterState m_state;
  
    bool m_pending_xon;
    bool m_pending_xoff;

    Ptr<TocinoNetDevice> m_tnd; // link to owning TocinoNetDevice

    std::vector< Ptr <CallbackQueue> > m_queues; // links to queues

    Ptr<TocinoChannel> m_channel; // link to channel

    void TransmitEnd(); // can this be private? needs to be invoked by Simulator::Schedule()
    uint32_t Arbitrate(); // returns linearized <port, vc> tuple
};

} // namespace ns3

#endif // __TOCINO_TX_H__
