/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_HELPER_H__
#define __TOCINO_HELPER_H__

#include "ns3/ptr.h"

#include "ns3/tocino-misc.h"

namespace ns3 {

class TocinoChannel;
class TocinoNetDevice;

void
TocinoChannelHelper( Ptr<TocinoNetDevice> tx_nd, TocinoOutputPort tx_port,
                     Ptr<TocinoNetDevice> rx_nd, TocinoInputPort rx_port );

}

#endif /* __TOCINO_HELPER_H__ */

