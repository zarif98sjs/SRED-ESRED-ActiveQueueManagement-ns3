/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;

static void
TraceQueue (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();
  uint32_t qMaxSize = queue->GetMaxSize ().GetValue (); 
  double bufferOccupancy = qSize * 100.0 / (double) qMaxSize; 
  std::ofstream q ("wired-sred-queue-100Leaf.dat", std::ios::out | std::ios::app);
  q << Simulator::Now ().GetSeconds () << " " << qSize << " " << bufferOccupancy << std::endl;
  Simulator::Schedule (Seconds (0.01), &TraceQueue, queue);
}

int main (int argc, char *argv[])
{
  uint32_t    nLeaf = 100;
  uint32_t    maxPackets = 100;
  bool        modeBytes  = true; // byte mode

  // queue disc limit * pktSize ~ 0.5 Mytes
  uint32_t    queueDiscLimitPackets = 977;
  uint32_t    pktSize = 512;

  std::string appDataRate = "650Mbps";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "45Mbps";
  std::string bottleNeckLinkDelay = "1ms";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the device queue", maxPackets);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set Queue disc mode to Packets (false) or bytes (true)", modeBytes);

  cmd.Parse (argc,argv);

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize",
                      StringValue (std::to_string (maxPackets) + "p"));

  if (!modeBytes)
    {
      Config::SetDefault ("ns3::StabilizedRedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }
  else
    {
      Config::SetDefault ("ns3::StabilizedRedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
    }

  ////////////////////////////////////////////////////////////////////////////////////

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));


  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                nLeaf, pointToPointLeaf,
                                bottleNeckLink);

  ////////////////////////////////////////////////////////////////////////////////////

  // Install Stack
  InternetStackHelper stack;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      stack.Install (d.GetLeft (i));
    }
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      stack.Install (d.GetRight (i));
    }

  stack.Install (d.GetLeft ());
  stack.Install (d.GetRight ());
  TrafficControlHelper tchBottleneck;
  QueueDiscContainer queueDiscs;
  tchBottleneck.SetRootQueueDisc ("ns3::StabilizedRedQueueDisc");
  tchBottleneck.Install (d.GetLeft ()->GetDevice (0));
  queueDiscs = tchBottleneck.Install (d.GetRight ()->GetDevice (0));

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  // clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  // clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (d.GetLeft (i)));
    }
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (6.0));

  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      // Create an on/off app sending packets to the left side
      AddressValue remoteAddress (InetSocketAddress (d.GetLeftIpv4Address (i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetRight (i)));
    }
  clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (5.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper ascii;
  bottleNeckLink.EnableAsciiAll (ascii.CreateFileStream ("wireless-sred.tr"));
  bottleNeckLink.EnablePcapAll ("wireless-sred");

  // // Flow monitor
  // Ptr<FlowMonitor> flowMonitor;
  // FlowMonitorHelper flowHelper;
  // flowMonitor = flowHelper.InstallAll();
  // Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, flowMonitor);
  Simulator::Schedule (Seconds (1), &TraceQueue, queueDiscs.Get (0));

  Simulator::Stop (Seconds (6.0)); // force stop,
  std::cout << "Running the simulation" << std::endl;
  Simulator::Run ();

  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();

  // if (st.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP) == 0)
  //   {
  //     std::cout << "There should be some unforced drops" << std::endl;
  //     exit (1);
  //   }

  // if (st.GetNDroppedPackets (QueueDisc::INTERNAL_QUEUE_DROP) != 0)
  //   {
  //     std::cout << "There should be zero drops due to queue full" << std::endl;
  //     exit (1);
  //   }

  std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;
  std::cout << st << std::endl;
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();
  return 0;
}
