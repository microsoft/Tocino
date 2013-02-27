/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_MISC_H__
#define __TOCINO_MISC_H__

#include <stdint.h>
#include <limits>
#include <ostream>

#include "tocino-type-safe-uint32.h"

#define STATIC_ASSERT( condition, name )\
    typedef char assert_failed_ ## name [ (condition) ? 1 : -1 ];

namespace ns3
{

//
// Directions
//

DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoDirection );

const TocinoDirection TOCINO_DIRECTION_POS( 0 );
const TocinoDirection TOCINO_DIRECTION_NEG( 1 );
const TocinoDirection TOCINO_INVALID_DIRECTION( std::numeric_limits<uint32_t>::max() );

//
// Dimensions
//

DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoDimension );

const TocinoDimension TOCINO_DIMENSION_X( 0 );
const TocinoDimension TOCINO_DIMENSION_Y( 1 );
const TocinoDimension TOCINO_DIMENSION_Z( 2 );
const TocinoDimension TOCINO_INVALID_DIMENSION( std::numeric_limits<uint32_t>::max() );

const uint32_t TOCINO_MAX_DIMENSIONS = 3;

//
// Ports
//

DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoPort );

struct TocinoInputPort : public TocinoPort {
    TocinoInputPort() : TocinoPort() {}
    TocinoInputPort( TocinoPort x ) : TocinoPort( x ) {}
    TocinoInputPort( uint32_t x ) : TocinoPort( x ) {}
};

struct TocinoOutputPort : public TocinoPort {
    TocinoOutputPort() : TocinoPort() {}
    TocinoOutputPort( TocinoPort x ) : TocinoPort( x ) {}
    TocinoOutputPort( uint32_t x ) : TocinoPort( x ) {}
};

const TocinoPort TOCINO_PORT_X_POS( 0 );
const TocinoPort TOCINO_PORT_X_NEG( 1 );
const TocinoPort TOCINO_PORT_Y_POS( 2 );
const TocinoPort TOCINO_PORT_Y_NEG( 3 );
const TocinoPort TOCINO_PORT_Z_POS( 4 );
const TocinoPort TOCINO_PORT_Z_NEG( 5 );
const TocinoPort TOCINO_PORT_HOST( 6 );
const TocinoPort TOCINO_INVALID_PORT( std::numeric_limits<uint32_t>::max() );

const uint32_t TOCINO_MAX_PORTS = 7;

//
// Virtual Channels
//

DEFINE_TOCINO_TYPE_SAFE_UINT32( TocinoVC );

struct TocinoInputVC : public TocinoVC {
    TocinoInputVC() : TocinoVC() {}
    TocinoInputVC( TocinoVC x ) : TocinoVC( x ) {}
    TocinoInputVC( uint32_t x ) : TocinoVC( x ) {}
};

struct TocinoOutputVC : public TocinoVC {
    TocinoOutputVC() : TocinoVC() {}
    TocinoOutputVC( TocinoVC x ) : TocinoVC( x ) {}
    TocinoOutputVC( uint32_t x ) : TocinoVC( x ) {}
};

const uint32_t TOCINO_INVALID_VC = std::numeric_limits<uint32_t>::max();

// We must ensure log2(MAX_VCS) == VC_BITS
const uint32_t TOCINO_NUM_VC_BITS = 4;
const uint32_t TOCINO_MAX_VCS = 16;

//
// Misc helper functions
//

void TocinoCustomizeLogging();

TocinoDirection TocinoGetDirection( const TocinoPort );
TocinoDimension TocinoGetDimension( const TocinoPort );
TocinoPort TocinoGetPort( const TocinoDimension, const TocinoDirection );

TocinoDirection TocinoGetOppositeDirection( const TocinoDirection );

std::string TocinoDirectionToString( const TocinoDirection );
std::string TocinoDimensionToString( const TocinoDimension );
std::string TocinoPortToString( const TocinoPort );

}

#endif //__TOCINO_MISC_H__
