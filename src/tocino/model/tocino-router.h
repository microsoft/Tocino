/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_ROUTER_H__
#define __TOCINO_ROUTER_H__   

#include "ns3/object.h"
#include "ns3/ptr.h"

#include "tocino-misc.h"

namespace ns3
{

class Packet;
class TocinoNetDevice;
class TocinoRx;

// ISSUE-REVIEW: consider moving this to its own file
struct TocinoRoute
{
    TocinoOutputPort outputPort;
    TocinoOutputVC outputVC;
    
    TocinoRoute( TocinoOutputPort port, TocinoOutputVC vc ) 
        : outputPort( port )
        , outputVC( vc )
    {}
    
    bool operator==( const TocinoRoute& other ) const
    {
        if( other.outputPort != outputPort )
            return false;

        if( other.outputVC != outputVC )
            return false;

        return true;
    }

    bool operator!=( const TocinoRoute& other ) const
    {
        return !( *this == other );
    }
};

struct TocinoRouter : public Object
{
    static TypeId GetTypeId( void );

    virtual TocinoRoute Route( const TocinoInputVC ) = 0;

    virtual TocinoRoute GetCurrentRoute( const TocinoInputVC ) const = 0;

    virtual void Initialize( Ptr<TocinoNetDevice>, const TocinoRx* ) = 0;
    
    static const TocinoRoute INVALID_ROUTE;
    static const TocinoRoute CANNOT_ROUTE;
};

}
#endif //__TOCINO_ROUTER_H__
