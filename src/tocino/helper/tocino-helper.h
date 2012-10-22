/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_HELPER_H__
#define __TOCINO_HELPER_H__

#include "ns3/ptr.h"

namespace ns3 {

class TocinoChannel;
class TocinoNetDevice;

Ptr<TocinoChannel>
TocinoChannelHelper( Ptr<TocinoNetDevice> tx_nd, uint32_t tx_port,
                     Ptr<TocinoNetDevice> rx_nd, uint32_t rx_port );

}

#endif /* __TOCINO_HELPER_H__ */

