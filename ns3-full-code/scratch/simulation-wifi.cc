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
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;


std::string dir;
uint32_t prev_b = 0;
Time prevTime = Seconds (0);

// Calculate throughput
static void
TraceThroughput (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr ("red-vs-ared-wifi-throughput.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << (8 * (itr->second.txBytes - prev_b)) / (1e6 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
  prevTime = curTime;
  prev_b = itr->second.txBytes;
  Simulator::Schedule (Seconds (0.4), &TraceThroughput, monitor);
}

NS_LOG_COMPONENT_DEFINE ("MyRedAredWifiExample");

int main (int argc, char *argv[])
{
  uint32_t    nLeaf = 10;
  uint32_t    maxPackets = 100;
  bool        modeBytes  = false;
  uint32_t    queueDiscLimitPackets = 1000;
  double      minTh = 5;
  double      maxTh = 15;
  uint32_t    pktSize = 512;
  std::string appDataRate = "10Mbps";
  std::string queueDiscType = "RED";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "7Mbps";
  std::string bottleNeckLinkDelay = "50ms";

  // newly added
  uint32_t nWifi = 3;

  // LogComponentEnableAll (LogLevel(LOG_LEVEL_ALL|LOG_PREFIX_FUNC));
  // LogComponentEnable ("OnOffApplication",LogLevel(LOG_LEVEL_INFO|LOG_PREFIX_FUNC));

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the device queue", maxPackets);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("queueDiscType", "Set Queue disc type to RED or ARED", queueDiscType);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set Queue disc mode to Packets (false) or bytes (true)", modeBytes);

  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);
  cmd.Parse (argc,argv);

  if ((queueDiscType != "RED") && (queueDiscType != "ARED"))
    {
      std::cout << "Invalid queue disc type: Use --queueDiscType=RED or --queueDiscType=ARED" << std::endl;
      exit (1);
    }

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize",
                      StringValue (std::to_string (maxPackets) + "p"));

  if (!modeBytes)
    {
      Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }
  else
    {
      Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
      minTh *= pktSize;
      maxTh *= pktSize;
    }

  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
  Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
  Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (pktSize));

  if (queueDiscType == "ARED")
    {
      // Turn on ARED
      Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (10.0));
    }

  /**
   * @brief 2 point to point nodes, which later will work as AP nodes for wifi
   * 
   */

  NodeContainer p2pBottleNeckNodes;
  p2pBottleNeckNodes.Create (2);

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  NetDeviceContainer p2pBottleNeckDevices;
  p2pBottleNeckDevices = bottleNeckLink.Install (p2pBottleNeckNodes);

  /**
   * @brief wifi left + right
   * 
   */

  NodeContainer wifiStaNodesLeft,wifiStaNodesRight;
  wifiStaNodesLeft.Create (nWifi);
  wifiStaNodesRight.Create (nWifi);
  NodeContainer wifiApNodeLeft = p2pBottleNeckNodes.Get (0);
  NodeContainer wifiApNodeRight = p2pBottleNeckNodes.Get (1);

  // PHY helper
  // constructs the wifi devices and the interconnection channel between these wifi nodes.
  YansWifiChannelHelper channelLeft = YansWifiChannelHelper::Default ();
  YansWifiChannelHelper channelRight = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phyLeft,phyRight;
  phyLeft.SetChannel (channelLeft.Create ()); //  all the PHY layer objects created by the YansWifiPhyHelper share the same underlying channel
  phyRight.SetChannel (channelRight.Create ()); 

  // MAC layer
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager"); // tells the helper the type of rate control algorithm to use, here AARF algorithm

  WifiMacHelper macLeft,macRight;
  Ssid ssidLeft = Ssid ("ns-3-ssid-left"); //  creates an 802.11 service set identifier (SSID) object
  Ssid ssidRight = Ssid ("ns-3-ssid-right"); 
  
  // configure Wi-Fi for all of our STA nodes
  macLeft.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidLeft),
               "ActiveProbing", BooleanValue (false)); 
               
  macRight.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidRight),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevicesLeft, staDevicesRight;
  staDevicesLeft = wifi.Install (phyLeft, macLeft, wifiStaNodesLeft);
  staDevicesRight = wifi.Install (phyRight, macRight, wifiStaNodesRight);

  //  configure the AP (access point) node
  macLeft.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssidLeft));
  macRight.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssidRight));

  NetDeviceContainer apDevicesLeft, apDevicesRight;
  apDevicesLeft = wifi.Install (phyLeft, macLeft, wifiApNodeLeft); // single AP which shares the same set of PHY-level Attributes (and channel) as the station
  apDevicesRight = wifi.Install (phyRight, macRight, wifiApNodeRight); 


  /**
   * 
   * Mobility Model
   * 
   * We want the STA nodes to be mobile, wandering around inside a bounding box, 
   * and we want to make the AP node stationary
   * 
   **/

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  // tell STA nodes how to move
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  // install on STA nodes
  mobility.Install (wifiStaNodesLeft);
  mobility.Install (wifiStaNodesRight);

  // tell AP node to stay still
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // install on AP node
  mobility.Install (wifiApNodeLeft);
  mobility.Install (wifiApNodeRight);

  // Install Stack
  InternetStackHelper stack;
  stack.Install (wifiApNodeLeft);
  stack.Install (wifiApNodeRight);
  stack.Install (wifiStaNodesLeft);
  stack.Install (wifiStaNodesRight);

  // install queue
  TrafficControlHelper tchBottleneck;
  QueueDiscContainer queueDiscLeft, queueDiscRight;
  tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
  queueDiscRight = tchBottleneck.Install (wifiApNodeRight.Get(0)->GetDevice(0));

  // Assign IP Addresses
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pBottleNeckDevices);

  Ipv4InterfaceContainer staNodeInterfacesLeft, staNodeInterfacesRight;
  Ipv4InterfaceContainer apNodeInterfaceLeft, apNodeInterfaceRight;

  address.SetBase ("10.1.2.0", "255.255.255.0");
  staNodeInterfacesLeft = address.Assign (staDevicesLeft);
  apNodeInterfaceLeft = address.Assign (apDevicesLeft);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  staNodeInterfacesRight = address.Assign (staDevicesRight);
  apNodeInterfaceRight = address.Assign (apDevicesRight);


  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
 
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < nWifi; ++i)
    {
      // create sink app on left side node
      sinkApps.Add (packetSinkHelper.Install (wifiStaNodesLeft.Get(i)));
    }
  sinkApps.Start (Seconds (2.0));
  sinkApps.Stop (Seconds (12.0));

  // sinkApps.Start (Seconds (0.0));
  // sinkApps.Stop (Seconds (30.0));

  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < nWifi; ++i)
    {
      // Create an on/off app on right side node which sends packets to the left side
      AddressValue remoteAddress (InetSocketAddress (staNodeInterfacesLeft.GetAddress(i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (wifiStaNodesRight.Get(i)));
    }
  clientApps.Start (Seconds (3.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (6.0)); // Stop before the sink  
  // clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  // clientApps.Stop (Seconds (17.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (14.0)); // force stop,

  AsciiTraceHelper ascii;
  bottleNeckLink.EnableAsciiAll (ascii.CreateFileStream ("red-vs-ared-wifi.tr"));
  bottleNeckLink.EnablePcapAll ("red-vs-ared-wifi.tr");

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, flowMonitor);

  std::cout << "Running the simulation :( " << std::endl;
  Simulator::Run ();

  flowMonitor->SerializeToXmlFile("red-vs-ared-wifi.xml", true, true);

  QueueDisc::Stats stRight = queueDiscRight.Get (0)->GetStats ();

  if (stRight.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP) == 0)
    {
      std::cout << stRight << std::endl;
      std::cout << "There should be some unforced drops (RIGHT)" << std::endl;
      exit (1);
    }

  if (stRight.GetNDroppedPackets (QueueDisc::INTERNAL_QUEUE_DROP) != 0)
    {
      std::cout << "There should be zero drops due to queue ful; (RIGHT)" << std::endl;
      exit (1);
    }

  std::cout << "*** Stats from the bottleneck queue disc (RIGHT) ***" << std::endl;
  std::cout << stRight << std::endl;
  
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();
  return 0;
}