/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-helper.h"

#include "ns3/object.h"

#include "ns3/tocino-channel.h"
#include "ns3/tocino-net-device.h"
#include "ns3/tocino-rx.h"

namespace ns3 {

void
TocinoChannelHelper( Ptr<TocinoNetDevice> tx_nd, uint32_t tx_port,
                     Ptr<TocinoNetDevice> rx_nd, uint32_t rx_port )
{
    Ptr<TocinoChannel> c = CreateObject<TocinoChannel>();
    

    tx_nd->SetChannel( tx_port, c );
    
    c->SetTransmitter( tx_nd->GetTransmitter( tx_port ) );
    c->SetReceiver( rx_nd->GetReceiver( rx_port ) );
}

}

