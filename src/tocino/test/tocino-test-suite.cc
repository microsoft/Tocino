/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "tocino-callbackqueue.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class TocinoTestSuite : public TestSuite
{
public:
  TocinoTestSuite ();
};

TocinoTestSuite::TocinoTestSuite ()
  : TestSuite ("tocino", UNIT)
{
  AddTestCase (new TocinoCallbackQueue);
}

// Do not forget to allocate an instance of this TestSuite
static TocinoTestSuite tocinoTestSuite;

