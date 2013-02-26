/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "tocino-helper.h"

#include "ns3/object.h"

#include "ns3/tocino-channel.h"
#include "ns3/tocino-net-device.h"
#include "ns3/tocino-rx.h"

namespace ns3 {

void
TocinoChannelHelper( Ptr<TocinoNetDevice> tx_nd, TocinoOutputPort tx_port,
                     Ptr<TocinoNetDevice> rx_nd, TocinoInputPort rx_port )
{
    uint32_t txPortNum = tx_port.AsUInt32();
    uint32_t rxPortNum = rx_port.AsUInt32();

    Ptr<TocinoChannel> c = CreateObject<TocinoChannel>();
    
    tx_nd->SetChannel( txPortNum, c );
    
    c->SetTransmitter( tx_nd->GetTransmitter( txPortNum ) );
    c->SetReceiver( rx_nd->GetReceiver( rxPortNum ) );
}

}

