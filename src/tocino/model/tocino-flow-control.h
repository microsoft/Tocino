/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_FLOW_CONTROL_H__
#define __TOCINO_FLOW_CONTROL_H__

#include <bitset>

#include "ns3/ptr.h"

#include "tocino-misc.h"

namespace ns3
{

class Packet;

typedef std::bitset<TOCINO_NUM_VC_BITS> TocinoVCBitSet;
typedef TocinoVCBitSet TocinoFlowControlState;

Ptr<Packet> GetTocinoFlowControlFlit( const TocinoFlowControlState& );
bool IsTocinoFlowControlFlit( Ptr<const Packet> );
TocinoFlowControlState GetTocinoFlowControlState( Ptr<const Packet> );

const TocinoFlowControlState TocinoAllXON( ~0 );
const TocinoFlowControlState TocinoAllXOFF( 0 );
}

#endif // __TOCINO_FLOW_CONTROL_H__
