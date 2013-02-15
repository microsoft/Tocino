/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_CROSSBAR_H__
#define __TOCINO_CROSSBAR_H__

#include "ns3/ptr.h"

#include "tocino-misc.h"
#include "tocino-router.h"

namespace ns3
{

class Packet;
class TocinoNetDevice;
class TocinoRx;
class TocinoRoutingTable;

// This class is responsible for forwarding flits
// from the input stage to the output stage

class TocinoCrossbar
{
    public:

    TocinoCrossbar(
            const TocinoNetDevice*,
            const TocinoInputPort );

    bool IsForwardable( const TocinoRoute ) const;
    
    void ForwardFlit( Ptr<Packet>, const TocinoRoute );

    private:
   
    bool TransmitterCanAcceptFlit( 
            const TocinoOutputPort,
            const TocinoOutputVC ) const;
  
    const TocinoNetDevice* m_tnd;
    const TocinoInputPort m_inputPort;
 
    typedef std::vector< TocinoInputVC > ForwardingTableVec;
    std::vector< ForwardingTableVec > m_forwardingTable;
    
    const TocinoInputVC& GetForwardingTableEntry( 
            const TocinoOutputPort,
            const TocinoOutputVC ) const;
    
    void SetForwardingTableEntry( 
            const TocinoOutputPort,
            const TocinoOutputVC,
            const TocinoInputVC );
    
    void ResetForwardingTableEntry( 
            const TocinoOutputPort,
            const TocinoOutputVC );
};

}

#endif // __TOCINO_CROSSBAR_H__
