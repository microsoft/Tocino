/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef _TOCINO_TYPE_SAFE_UINT32_H__
#define _TOCINO_TYPE_SAFE_UINT32_H__

#include <stdint.h>
#include <limits>
#include <ostream>

// A thin wrapper around an unsigned integer.
//
// The advantage of this is that different instances
// of this class template are not implicitly convertible
// to each other, unlike a simple typedef uint32_t.
//
// There is a trade-off here between the convenience of
// implicit conversions and the restriction of possible
// errors.
//
// Note to maintainers:
//
// I tried to define the minimum number of operators
// here in order to get things to compile.
//
// You can can always accomplish whatever you want by
// calling AsUInt32 and doing things the long way.  
// *This is a feature.*
//
// The more operators we define below, the more code
// will compile silently without requiring scrutiny.
//
// -MAS

#define DEFINE_TOCINO_TYPE_SAFE_UINT32(name) \
    struct name##Base {}; \
    typedef TocinoTypeSafeUInt32<name##Base> name;

template< typename T > // Intentionally never used
class TocinoTypeSafeUInt32
{
    private:

    uint32_t m_val;

    public:
    
    TocinoTypeSafeUInt32()
        : m_val( std::numeric_limits<uint32_t>::max() )
    {}

    // ISSUE-REVIEW:
    // This could be declared "explicit" for even more control
    TocinoTypeSafeUInt32( uint32_t val )
        : m_val( val )
    {}

    //operator uint32_t() const
    uint32_t AsUInt32() const
    {
        return m_val;
    }
    
    TocinoTypeSafeUInt32& operator++ () // Prefix
    {
        m_val++;
        return *this;
    }

    TocinoTypeSafeUInt32 operator++ (int) // Postfix
    {
        TocinoTypeSafeUInt32 x = *this;
        ++(*this);
        return x;
    }
   
    bool operator< ( const TocinoTypeSafeUInt32& other ) const
    {
        return m_val < other.m_val;
    }
    
    bool operator== ( const uint32_t v ) const
    {
        return m_val == v;
    }
    
    bool operator!= ( const uint32_t v ) const
    {
        return m_val != v;
    }
    
    template< typename OT >
    bool operator== ( const TocinoTypeSafeUInt32<OT>& other ) const
    {
        return m_val == other.AsUInt32();
    }
   
    template< typename OT >
    bool operator!= ( const TocinoTypeSafeUInt32<OT>& other ) const
    {
        return m_val != other.AsUInt32();
    }
};

template< typename T >
bool operator< (
        const uint32_t lhs, 
        const TocinoTypeSafeUInt32<T>& rhs )
{
    return lhs < rhs.AsUInt32();
}

template< typename T >
bool operator== (
        const uint32_t lhs, 
        const TocinoTypeSafeUInt32<T>& rhs )
{
    return lhs == rhs.AsUInt32();
}

template< typename T >
std::ostream& operator<< (
        std::ostream& out,
        const TocinoTypeSafeUInt32<T>& x )
{
    out << x.AsUInt32();
    return out;
}

#endif // _TOCINO_TYPE_SAFE_UINT32_H__
