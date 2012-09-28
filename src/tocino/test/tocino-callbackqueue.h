#ifndef TOCINO_CALLBACKQUEUE_H
#define TOCINO_CALLBACKQUEUE_H

#include "ns3/test.h"
using namespace ns3;

class TocinoCallbackQueue : public TestCase
{
public:
  TocinoCallbackQueue();
  virtual ~TocinoCallbackQueue();
private:
  virtual void DoRun (void);

};
#endif // TOCINO_CALLBACKQUEUE_H

