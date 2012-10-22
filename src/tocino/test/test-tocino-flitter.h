#ifndef __TEST_TOCINO_FLITTER_H__
#define __TEST_TOCINO_FLITTER_H__

#include "ns3/test.h"

namespace ns3 {

class TestTocinoFlitter : public TestCase
{
    public:

    TestTocinoFlitter();
    virtual ~TestTocinoFlitter();

    private:

    void TestEmpty();
    void TestOneFlit( unsigned );
    void TestTwoFlits( unsigned );
    void TestThreeFlits( unsigned );

    virtual void DoRun( void );
};

}
#endif // __TEST_TOCINO_FLITTER_H__
