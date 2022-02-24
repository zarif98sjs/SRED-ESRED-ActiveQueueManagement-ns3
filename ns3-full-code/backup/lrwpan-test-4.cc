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


#include <fstream>
#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include <ns3/lr-wpan-error-model.h>

//flow monitor
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

// Default Network Topology
//
// 
//                        
//  *           *    *    *
//  |           |    |    |  
// n_n_total/2-1... n1   n0   R0  --------- R1   n_(n_total/2) ...   n_n_Total
//                             point-to-point          |       |         |    
//                                                     *       *         *    
//                                                                 


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("lrwpan-simulation");

std::vector<Ptr<PacketSink>> sink_app;
std::string dir = "lrpan-results/";

uint32_t prev_b = 0;
Time prevTime = Seconds (0);

// // Calculate throughput
// static void
// TraceThroughput (Ptr<FlowMonitor> monitor)
// {
//   FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
//   auto itr = stats.begin ();
//   Time curTime = Now ();
//   std::ofstream thr (dir+"/throughput.dat", std::ios::out | std::ios::app);
//   thr <<  curTime << " " << (8 * (itr->second.txBytes - prev_b)) / (1.0 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
//   prevTime = curTime;
//   prev_b = itr->second.txBytes;
//   Simulator::Schedule (Seconds (0.1), &TraceThroughput, monitor);
// }

// Calculate throughput
static void
TraceThroughput (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (dir+"/throughput.dat", std::ios::out | std::ios::app);
  // thr <<  curTime << " " << (8 * (itr->second.txBytes)) / (1.0 * (curTime.GetSeconds ())) << std::endl;
   thr <<  curTime << " " << (8 * (itr->second.txBytes - prev_b)) / (1.0 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceThroughput, monitor);
}

// calculate delay
static void
TraceDelay (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (dir+"/delay.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << itr->second.delaySum.GetSeconds () / itr->second.rxPackets << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceDelay, monitor);
}

// calculate packet drop ratio
static void
TraceDrop (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (dir+"/drop.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << itr->second.lostPackets*100.0 / (itr->second.lostPackets + itr->second.rxPackets) << " " << itr->second.lostPackets*100.0 / (itr->second.txPackets) << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceDrop, monitor);
}

// calculate packet delivery ratio
static void
TraceDelivery (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (dir+"/delivery.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << itr->second.rxPackets*100.0 / (itr->second.lostPackets + itr->second.rxPackets) << " " << itr->second.rxPackets*100.0 / (itr->second.txPackets) << std::endl;
  // thr <<  curTime  std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceDelivery, monitor);
}

// calculate packets sent
static void
TraceSent (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (dir+"/sent.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << itr->second.txPackets << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceSent, monitor);
}


int
main (int argc, char *argv[])
{
  int n_total = 10;
  int n_flow = 10;
  double n_pkts_sec = 10;
  int n_speed = 25;
  int pkt_sz;

  double start_time = 0;
  double stop_time = 100;

  std::string link_speed = "50Mbps";
  std::string link_delay = "100ns";

  std::string tcpTypeId = "TcpNewReno";
  std::string queueDisc = "FifoQueueDisc";
  uint32_t delAckCount = 2;
  std::string recovery = "ns3::TcpClassicRecovery";

  bool enablePcap = false;
  Time stopTime = Seconds (stop_time);

  CommandLine cmd (__FILE__);

  cmd.AddValue ("tcp_type", "Transport protocol to use: TcpNewReno, TcpBbr", tcpTypeId);
  cmd.AddValue ("delAckCount", "Delayed ACK count", delAckCount);
  cmd.AddValue ("enablePcap", "Enable/Disable pcap file generation", enablePcap);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
  cmd.AddValue ("n_total", "total number of wifi nodes on both sides", n_total);
  cmd.AddValue ("n_flow", "total number of flows to generate", n_flow);
  cmd.AddValue ("n_pkts_sec", "total number of packets sent per second", n_pkts_sec);
  cmd.AddValue ("n_speed", "speed of wifi nodes on both sides", n_speed);

  cmd.Parse (argc, argv);

  queueDisc = std::string ("ns3::") + queueDisc;

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",TypeIdValue (TypeId::LookupByName (recovery)));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4194304));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (6291456));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (2));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("20p")));
  Config::SetDefault (queueDisc + "::MaxSize", QueueSizeValue (QueueSize ("100p")));

  cout<<"total nodes = "<<n_total<<endl;
  cout<<"total flows = "<<n_flow<<endl;
  cout<<"packets per second = "<<n_pkts_sec<<endl;
  cout<<"speed = "<<n_speed<<endl;

  //create nodes

  NodeContainer left_nodes, left_nodes_ap, right_nodes, right_nodes_ap, routers;
  
  routers.Create(2);
  left_nodes.Add(routers.Get(0));
  left_nodes.Create(n_total/2);
  right_nodes.Add(routers.Get(1));
  right_nodes.Create(n_total/2);

  // Ptr<LrWpanErrorModel> lrWpanError = CreateObject<LrWpanErrorModel> ();
  // for( int i=0; i<(n_total+2); i++ ) {
  //   std::string path = "/NodeList/";
  //   path += std::to_string(i);
  //   path += "/DeviceList/0/$ns3::LrWpanNetDevice/Phy/$ns3::LrWpanPhy/ErrorModel";
  //   Config::Set(path, PointerValue(lrWpanError));
  // }

  //add error?

  // PointToPointHelper link;

  // link.SetDeviceAttribute("DataRate", StringValue(link_speed));
  // link.SetChannelAttribute("Delay", StringValue(link_delay));

  CsmaHelper link;

  link.SetChannelAttribute("DataRate", StringValue(link_speed));
  link.SetChannelAttribute("Delay", StringValue(link_delay));

  NetDeviceContainer link_net = link.Install(routers);

  //setup wpan nodes

  MobilityHelper mobility_left;
  // mobility_left.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobility_left.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                "MinX", DoubleValue (0.0),
  //                                "MinY", DoubleValue (0.0),
  //                                "DeltaX", DoubleValue (80),
  //                                "DeltaY", DoubleValue (80),
  //                                "GridWidth", UintegerValue (10),
  //                                "LayoutType", StringValue ("RowFirst"));
  // mobility_left.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_left.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator");

  mobility_left.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant="+to_string(n_speed)+".0]"),
                                 "Bounds", RectangleValue (Rectangle (-1, 1, -1, 1)));
  for(int i = 1; i < n_total/2; i++)
  {
    mobility_left.Install (left_nodes.Get(i));
  }
  // mobility_left.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobility_left.Install (left_nodes.Get(0));

  LrWpanHelper lrWpanHelper_left;
  NetDeviceContainer lrwpanDevices_left = lrWpanHelper_left.Install (left_nodes);
  //NetDeviceContainer lrwpanDevices_left_ap = lrWpanHelper_left.Install(left_nodes_ap);

  lrWpanHelper_left.AssociateToPan (lrwpanDevices_left, 0);
  //lrWpanHelper_left.AssociateToPan (lrwpanDevices_left_ap, 0);

  SixLowPanHelper sixLowPanHelper_left;
  NetDeviceContainer sixLowPanDevices_left = sixLowPanHelper_left.Install (lrwpanDevices_left);
  //NetDeviceContainer sixLowPanDevices_left_ap = sixLowPanHelper_left.Install (lrwpanDevices_left_ap);

  for (uint32_t i = 0; i < sixLowPanDevices_left.GetN (); i++)
    {
      Ptr<NetDevice> dev = sixLowPanDevices_left.Get (i);
      dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
      dev->SetAttribute ("MeshUnderRadius", UintegerValue (20));
    }
  // for (uint32_t i = 0; i < lrwpanDevices_left.GetN (); i++)
  //   {
  //     Ptr<NetDevice> dev = lrwpanDevices_left.Get (i);
  //     dev->SetAttribute ("ReceiveErrorModel", PointerValue (lrWpanError));
  //   }


  MobilityHelper mobility_right;
  // mobility_right.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobility_right.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                "MinX", DoubleValue (0.0),
  //                                "MinY", DoubleValue (0.0),
  //                                "DeltaX", DoubleValue (80),
  //                                "DeltaY", DoubleValue (80),
  //                                "GridWidth", UintegerValue (10),
  //                                "LayoutType", StringValue ("RowFirst"));
  // mobility_right.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_right.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator");

  mobility_right.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant="+to_string(n_speed)+".0]"),
                                 "Bounds", RectangleValue (Rectangle (-1, 1, -1, 1)));
  for(int i = 1; i < n_total/2; i++)
  {
    mobility_right.Install (right_nodes.Get(i));
  }
  
  // mobility_right.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobility_right.Install (right_nodes.Get(0));

  LrWpanHelper lrWpanHelper_right;
  NetDeviceContainer lrwpanDevices_right = lrWpanHelper_right.Install (right_nodes);
  //NetDeviceContainer lrwpanDevices_right_ap = lrWpanHelper_right.Install(right_nodes_ap);

  lrWpanHelper_right.AssociateToPan (lrwpanDevices_right, 0);
  //lrWpanHelper_right.AssociateToPan (lrwpanDevices_right_ap, 0);

  SixLowPanHelper sixLowPanHelper_right;
  NetDeviceContainer sixLowPanDevices_right = sixLowPanHelper_right.Install (lrwpanDevices_right);
  //NetDeviceContainer sixLowPanDevices_right_ap = sixLowPanHelper_right.Install (lrwpanDevices_right_ap);

  for (uint32_t i = 0; i < sixLowPanDevices_right.GetN (); i++)
    {
      Ptr<NetDevice> dev = sixLowPanDevices_right.Get (i);
      dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
      dev->SetAttribute ("MeshUnderRadius", UintegerValue (20));
    }
  // for (uint32_t i = 0; i < lrwpanDevices_right.GetN (); i++)
  //   {
  //     Ptr<NetDevice> dev = lrwpanDevices_right.Get (i);
  //     dev->SetAttribute ("ReceiveErrorModel", PointerValue (lrWpanError));
  //   }


  InternetStackHelper stack;
  stack.Install(left_nodes);
  stack.Install(right_nodes);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:aaaa::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer left_interface;
  left_interface = ipv6.Assign(sixLowPanDevices_left);
  left_interface.SetForwarding (0, true);
  left_interface.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2001:bbbb::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer right_interface;
  right_interface = ipv6.Assign(sixLowPanDevices_right);
  right_interface.SetForwarding (0, true);
  right_interface.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2001:cccc::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer link_interface;
  link_interface = ipv6.Assign(link_net);
  //link_interface.SetDefaultRoute(0,1);
  //link_interface.SetDefaultRoute(1,0);
  link_interface.SetForwarding (1, true);
  link_interface.SetDefaultRouteInAllNodes (1);
  link_interface.SetForwarding (0, true);
  link_interface.SetDefaultRouteInAllNodes (0);

  uint32_t mtu_bytes = 160;
  Header* temp_header = new Ipv6Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  pkt_sz = tcp_adu_size;
  cout<<"adu "<<tcp_adu_size<<endl;

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));

  srand(time(0));
  int j = 0;

  for(j = 0; j < n_flow/4; j++)
  {
    int sender = ((rand() % (n_total/2-1)))+1;
    int receiver = ((rand() % (n_total/2-1)))+1;
    //int sender = i+1;
    //int receiver = i+1;
    
    // Select sender side port
    uint16_t port = 5000+j;
    //uint16_t port = 50000;

    // Install application on the sender
    OnOffHelper source ("ns3::TcpSocketFactory", Inet6SocketAddress (right_interface.GetAddress (receiver, 1), port));
    //BulkSendHelper source ("ns3::TcpSocketFactory", Inet6SocketAddress (link_interface.GetAddress (1, 1), port));
    source.SetAttribute ("PacketSize", UintegerValue (pkt_sz));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetConstantRate (DataRate (n_pkts_sec*pkt_sz*8));
    ApplicationContainer sourceApps = source.Install (left_nodes.Get (sender));
    //ApplicationContainer sourceApps = source.Install (right_nodes.Get (sender));
    sourceApps.Start (Seconds (start_time));
    //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
    sourceApps.Stop (stopTime);

    // Install application on the receiver
    PacketSinkHelper sink ("ns3::TcpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (right_nodes.Get (receiver));
    //ApplicationContainer sinkApps = sink.Install (routers.Get (1));

    Ptr<PacketSink> temp = StaticCast<PacketSink> (sinkApps.Get (0));

    sink_app.push_back(temp);
    sinkApps.Start (Seconds (start_time));
    sinkApps.Stop (stopTime);

    //throughput calculation
    //Simulator::Schedule (Seconds (0.1), &TraceThroughput, i);
  }

  for(j = j; j < n_flow/2; j++)
  {
    int sender = ((rand() % (n_total/2-1)))+1;
    int receiver = ((rand() % (n_total/2-1)))+1;
    //int sender = i+1;
    //int receiver = i+1;
    
    // Select sender side port
    uint16_t port = 5000+j;
    //uint16_t port = 50000;

    // Install application on the sender
    OnOffHelper source ("ns3::TcpSocketFactory", Inet6SocketAddress (left_interface.GetAddress (receiver, 1), port));
    //BulkSendHelper source ("ns3::TcpSocketFactory", Inet6SocketAddress (link_interface.GetAddress (1, 1), port));
    source.SetAttribute ("PacketSize", UintegerValue (pkt_sz));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetConstantRate (DataRate (n_pkts_sec*pkt_sz*8));
    ApplicationContainer sourceApps = source.Install (right_nodes.Get (sender));
    //ApplicationContainer sourceApps = source.Install (right_nodes.Get (sender));
    sourceApps.Start (Seconds (start_time));
    //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
    sourceApps.Stop (stopTime);

    // Install application on the receiver
    PacketSinkHelper sink ("ns3::TcpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (left_nodes.Get (receiver));
    //ApplicationContainer sinkApps = sink.Install (routers.Get (1));

    Ptr<PacketSink> temp = StaticCast<PacketSink> (sinkApps.Get (0));

    sink_app.push_back(temp);
    sinkApps.Start (Seconds (start_time));
    sinkApps.Stop (stopTime);

    //throughput calculation
    //Simulator::Schedule (Seconds (0.1), &TraceThroughput, i);
  }

  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, sizeof (buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
  std::string currentTime (buffer);

  
  std::string dirToDel = "rm -rf " + dir;
  if (system (dirToDel.c_str ()) == -1)
  {
      exit (1);
  }
  std::string dirToSave = "mkdir -p " + dir;
  if (system (dirToSave.c_str ()) == -1)
  {
    exit (1);
  }


    // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  
  Simulator::Schedule (Seconds (0.1), &TraceThroughput, flowMonitor);
    Simulator::Schedule (Seconds (0.1), &TraceDelay, flowMonitor);
    Simulator::Schedule (Seconds (0.1), &TraceDrop, flowMonitor);
    Simulator::Schedule (Seconds (0.1), &TraceDelivery, flowMonitor);
    Simulator::Schedule (Seconds (0.1), &TraceSent, flowMonitor);
  //Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);

  Simulator::Stop (stopTime);

  Simulator::Run ();
  flowHelper.SerializeToXmlFile(dir+"/lrpan.xml", false, false);
  //std::vector<double> averageThroughput;
  // for(int i = 0; i < 5; i++)
  // {
  //   averageThroughput.push_back((sink_app[i]->GetTotalRx () * 8.0)/(1 * stop_time));
  //   cout<<sink_app[i]->GetTotalRx ()<<endl;
  // }
  Simulator::Destroy ();
  // double total_throughput = 0;
  // for(int i = 0; i < 5; i++)
  // {
  //   std::cout << "\nAverage throughput: (flowpair "<<i <<") "<< averageThroughput[i]<< " bit/s" << std::endl;
  //   total_throughput+=averageThroughput[i];
  // }
  // std::cout << "\nAverage throughput: " << (total_throughput/n_flow) << " bit/s" << std::endl;
  return 0;


}
