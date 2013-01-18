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
    TocinoInputVC inputVC;
    TocinoOutputVC outputVC;
   
    TocinoRoute()
        : outputPort( TOCINO_INVALID_PORT )
        , inputVC( TOCINO_INVALID_VC )
        , outputVC( TOCINO_INVALID_VC )
    {}
    
    TocinoRoute( 
            TocinoOutputPort op, 
            TocinoInputVC ivc,
            TocinoOutputVC ovc ) 
        : outputPort( op )
        , inputVC( ivc )
        , outputVC( ovc )
    {}
    
    bool operator==( const TocinoRoute& other ) const
    {
        if( other.outputPort != outputPort )
            return false;
        
        if( other.inputVC != inputVC )
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

const TocinoRoute TOCINO_INVALID_ROUTE(
            TOCINO_INVALID_PORT,
            TOCINO_INVALID_VC,
            TOCINO_INVALID_VC );

struct TocinoRouter : public Object
{
    static TypeId GetTypeId( void );

    // ISSUE-REVIEW:
    // Ideally we'd only have only the route function
    // as a part of this abstract interface.  Anything that
    // can take a flit and produce a route should be a legit
    // router.  If today's TocinoRx needs more, maybe he
    // should include a polymorphic downcast to a subclass?
    //
    // RELATED-ISSUE-REVIEW:
    // Do we really want to *require* that Route be a const
    // function?  I mean it's fine if an implementation wants
    // to do this, but it shouldn't be required.  But, how
    // the hell do you define an abstract base class where
    // derived classes can choose?
    virtual void Initialize(
            Ptr<TocinoNetDevice>, 
            const TocinoRx* ) = 0;
    
    virtual TocinoRoute Route( Ptr<const Packet> ) const = 0;
};

}
#endif //__TOCINO_ROUTER_H__
