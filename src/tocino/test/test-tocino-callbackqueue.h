#ifndef __TEST_TOCINO_CALLBACKQUEUE_H_
#define __TEST_TOCINO_CALLBACKQUEUE_H_

#include "ns3/test.h"
using namespace ns3;

class TestTocinoCallbackQueue : public TestCase
{
public:
  TestTocinoCallbackQueue();
  virtual ~TestTocinoCallbackQueue();
private:
  virtual void DoRun (void);

};
#endif // __TEST_TOCINO_CALLBACKQUEUE_H_

