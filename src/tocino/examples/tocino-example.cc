/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/tocino-helper.h"

#include "ns3/tocino-net-device.h"
#include "ns3/tocino-channel.h"

using namespace ns3;


int 
main (int argc, char *argv[])
{
  bool verbose = true;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc,argv);

  TocinoNetDevice tnd0;
  TocinoNetDevice tnd1;
  Ptr<TocinoChannel> tc = CreateObject<TocinoChannel>();

  tnd0.AddChannel( tc );
  tnd1.AddChannel( tc );

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


