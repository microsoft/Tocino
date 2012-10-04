#ifndef __TOCINO_TEST_FLITTER_DEFLITTER_H__
#define __TOCINO_TEST_FLITTER_DEFLITTER_H__

#include "ns3/test.h"

namespace ns3 {

class TestFlitter : public TestCase
{
    public:

    TestFlitter();
    virtual ~TestFlitter();

    private:

    void TestEmpty();
    void TestOneFlit( unsigned );
    void TestTwoFlits( unsigned );
    void TestThreeFlits( unsigned );

    virtual void DoRun( void );
};

class TestDeflitter : public TestCase
{
    public:

    TestDeflitter();
    virtual ~TestDeflitter();

    private:

    void TestOneFlit( unsigned );
    //void TestTwoFlits( unsigned );
    void TestThreeFlits( unsigned );

    virtual void DoRun( void );
};

}

#endif // __TOCINO_TEST_FLITTER_DEFLITTER_H__

