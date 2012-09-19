/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_CHANNEL_H__
#define __TOCINO_CHANNEL_H__

#include "ns3/channel.h"

namespace ns3 {

class Packet;
class TocinoNetDevice;

class TocinoChannel : public Channel
{
    public:

    static TypeId GetTypeId();

    TocinoChannel();
    
    void Attach( Ptr<TocinoNetDevice> device );

    bool TransmitStart( Ptr<Packet> p, uint32_t srcId );
    //virtual bool TransmitStart (Ptr<Packet> p, Ptr<TocinoNetDevice> src, Time txTime);

    virtual uint32_t GetNDevices() const;

    virtual Ptr<NetDevice> GetDevice( uint32_t i ) const;
    Ptr<TocinoNetDevice> GetTocinoNetDevice( uint32_t i ) const;
  
    //DataRate GetDataRate( void );
    //Time GetDelay( void );

    private:

    // disable copy and copy-assignment
    TocinoChannel& operator=( const TocinoChannel& );
    TocinoChannel( const TocinoChannel& );

    static const int N_DEVICES = 2;
    int32_t m_nDevices;

    //Time m_delay;
    //DataRate m_bps;

    class Link
    {
        public:
        Link() : m_src(0), m_dst(0) {}
        Ptr<TocinoNetDevice> m_src;
        Ptr<TocinoNetDevice> m_dst;
    };

    Link m_link[N_DEVICES];
};

}

#endif /* __TOCINO_CHANNEL_H__ */

