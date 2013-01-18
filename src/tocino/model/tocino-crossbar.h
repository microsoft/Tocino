/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_CROSSBAR_H__
#define __TOCINO_CROSSBAR_H__

#include "ns3/ptr.h"

#include "tocino-misc.h"
#include "tocino-forwarding-table.h"

namespace ns3
{

class Packet;
class TocinoNetDevice;
class TocinoRx;
class TocinoRoute;
class TocinoRouter;

// This class is responsible for forwarding flits
// from the input stage to the output stage

class TocinoCrossbar
{
    public:

    TocinoCrossbar( Ptr<TocinoNetDevice>, TocinoRx* );

    TocinoRoute FindForwardableRoute() const;
    
    static const TocinoRoute NO_FORWARDABLE_ROUTE;
    
    void ForwardFlit( Ptr<Packet>, TocinoRoute );

    const TocinoForwardingTable& GetForwardingTable() const;

    private:
    
    bool ForwardingInProgress( 
            const TocinoInputVC,
            const TocinoOutputPort,
            const TocinoOutputVC ) const;

    bool TransmitterCanAcceptFlit( 
            const TocinoOutputPort,
            const TocinoOutputVC ) const;
  
    const Ptr<TocinoNetDevice> m_tnd;
    TocinoRx* m_trx;

    TocinoForwardingTable m_forwardingTable;
    Ptr<TocinoRouter> m_router;
};

}

#endif // __TOCINO_CROSSBAR_H__
