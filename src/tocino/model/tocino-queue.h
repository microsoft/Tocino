/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_QUEUE_H__
#define __TOCINO_QUEUE_H__

#include <deque>

#include "ns3/assert.h"

namespace ns3
{

// A queue with IsAlmostFull().
//
// We do not inherit from the abstract base class
// ns3::Queue, because that would constrain what
// can be held in our queue.  The prototype for
// Dequeue must return a Ptr<Packet>, for example.
//
// We do not inherit from Object, because the ns3
// attribute system seems incompatible with class
// templates.

template< typename T >
class TocinoQueue 
{
    private:
   
    uint32_t m_maxDepth;
    uint32_t m_reserve;

    std::deque<T> m_queue;
    
    public:

    typedef T value_type;
    typedef const T& const_reference;
    
    static const uint32_t DEFAULT_MAX_DEPTH = 8;
    static const uint32_t DEFAULT_RESERVE = 0;
  
    TocinoQueue()
        : m_maxDepth( DEFAULT_MAX_DEPTH )
        , m_reserve( DEFAULT_RESERVE )
        , m_queue()
    {}

    void SetMaxDepth( uint32_t depth )
    {
        NS_ASSERT( depth >= m_queue.size() );
        m_maxDepth = depth;
    }

    void SetReserve( uint32_t reserve )
    {
        NS_ASSERT( reserve < m_maxDepth );
        m_reserve = reserve;
    }

    bool IsFull() const
    {
        return m_queue.size() == m_maxDepth;
    }

    bool IsEmpty() const
    {
        return m_queue.size() == 0;
    }

    uint32_t Size() const
    {
        return m_queue.size();
    }

    private:

    uint32_t RemainingEntries() const
    {
        return m_maxDepth - m_queue.size();
    }

    public:

    bool IsAlmostFull() const
    {
        if( RemainingEntries() > m_reserve )
        {
            return false;
        }

        return true;
    }

    void Enqueue( const T& v )
    {
        NS_ASSERT( !IsFull() );

        m_queue.push_back(v); 
    }

    value_type Dequeue()
    {
        NS_ASSERT( !IsEmpty() );

        value_type v = m_queue.front();
        m_queue.pop_front();

        return v;
    }

    const_reference PeekFront() const
    {
        NS_ASSERT( !IsEmpty() );

        return m_queue.front();
    }
    
    const_reference At( uint32_t idx ) const
    {
        NS_ASSERT( idx < m_queue.size() );

        return m_queue[idx];
    }
};

}

#endif // __TOCINO_QUEUE_H__

