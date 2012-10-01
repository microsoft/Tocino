/* -*- Mode:C++; c-file-style:"stroustrup"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_CHANNEL_H__
#define __TOCINO_CHANNEL_H__

#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/net-device.h"

#include "tocino-enum.h"
#include "tocino-net-device.h"
#include "tocino-net-device-receiver.h"
#include "tocino-net-device-transmitter.h"

namespace ns3 {
    class TocinoChannel : public Channel
    {
    public:
        static TypeId GetTypeId();
    
        TocinoChannel() { m_state = IDLE;}
        ~TocinoChannel() {};
    
        bool TransmitStart (Ptr<Packet> p);
        Time GetTransmissionTime(Ptr<Packet> p);
        
        void SetTransmitter(Ptr<TocinoNetDeviceTransmitter> tx) {m_tx = tx;}
        void SetReceiver(Ptr<TocinoNetDeviceReceiver> rx) {m_rx = rx;}
        uint32_t GetNDevices() const {return 2;}
        
        Ptr<NetDevice> GetDevice(uint32_t i) const;
        
    private:
        void TransmitEnd ();
        
        // channel parameters
        Time m_delay;
        DataRate m_bps;
        
        Ptr<Packet> m_packet;
        
        Ptr<TocinoNetDeviceTransmitter> m_tx;
        Ptr<TocinoNetDeviceReceiver> m_rx;
        TocinoChannelState m_state;
    };
} // namespace ns3

#endif /* __TOCINO_CHANNEL_H__ */

