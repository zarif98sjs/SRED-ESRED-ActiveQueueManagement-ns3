/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS); // sets the time resolution to one nanosecond
  LogComponentEnable ("UdpEchoClientApplication", LogLevel(LOG_LEVEL_ALL|LOG_PREFIX_FUNC)); //  enable logging components that is built into the Echo Client
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);

  NodeContainer nodes; // node is end-device : pc
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes); //  we will ask the PointToPointHelper to do the work involved in creating, configuring and installing our devices for us

  InternetStackHelper stack;
  stack.Install (nodes); // When it is executed, it will install an Internet Stack (TCP, UDP, IP, etc.) on each of the nodes in the node container.

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0"); // declare an address helper object and tell it that it should begin allocating IP addresses from the network 10.1.1.0 using the mask 255.255.255.0 to defifine the allocatable bits

  /**
      By default the addresses allocated will start at one and increase
      monotonically, so the first address allocated from this base will be 
      10.1.1.1, followed by 10.1.1.2, etc. 
   **/ 

  Ipv4InterfaceContainer interfaces = address.Assign (devices); // performs the address assignment

  UdpEchoServerHelper echoServer (9); // a port number that a client also knows , or you can set the “Port” Attribute to another value later using SetAttribute.

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9); // attributes set in constructors : RemoteAddress, RemotePort
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1)); // maximum number of packets we allow it to send during the simulation
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
