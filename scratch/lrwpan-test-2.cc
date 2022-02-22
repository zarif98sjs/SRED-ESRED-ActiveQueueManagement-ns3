#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/traffic-control-module.h"
#include <ns3/lr-wpan-error-model.h>
using namespace ns3;
int tracedNode=1;

/////////////////////////////////////////////

std::string exp_name = "lrwpan";

std::string dir;
uint32_t prev_b = 0;
Time prevTime = Seconds (0);

// // Calculate throughput
// static void
// TraceThroughput (Ptr<FlowMonitor> monitor)
// {
//   FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
//   auto itr = stats.begin ();
//   Time curTime = Now ();
//   std::ofstream thr (exp_name+"/throughput.dat", std::ios::out | std::ios::app);
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
  std::ofstream thr (exp_name+"/throughput.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  double tot_thr = 0;
  for(auto itr:stats)
  {
    double flow_thr = (8 * itr.second.rxBytes ) / (1.0 * curTime.GetSeconds () );
    tot_thr += flow_thr;
  }
  thr <<  curTime << " " << tot_thr << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceThroughput, monitor);
}

// calculate delay
static void
TraceDelay (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  std::ofstream thr (exp_name+"/delay_avg.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  double tot_delay = 0;
  int num_flows = 0;
  for(auto itr:stats)
  {
    tot_delay += itr.second.delaySum.GetSeconds ();
    num_flows++;
  }
  thr <<  curTime << " " << tot_delay/num_flows << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceDelay, monitor);
}

// calculate packet drop ratio
static void
TraceDrop (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  std::ofstream thr (exp_name+"/drop_avg.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  double tot_drop = 0;
  int num_flows = 0;
  for(auto itr:stats)
  {
    tot_drop += (100.0*itr.second.lostPackets/itr.second.txPackets);
    num_flows++;
  }
  thr <<  curTime << " " << tot_drop/num_flows << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceDrop, monitor);
}

// calculate packet delivery ratio
static void
TraceDelivery (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  std::ofstream thr (exp_name+"/delivery_avg.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  double tot_delivery = 0;
  int num_flows = 0;
  for(auto itr:stats)
  {
    tot_delivery += (100.0*itr.second.rxPackets/itr.second.txPackets);
    num_flows++;
  }
  thr <<  curTime << " " << tot_delivery/num_flows << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceDelivery, monitor);
}

// calculate total packets sent
static void
TraceSent (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  std::ofstream thr (exp_name+"/sent_total.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  double tot_sent = 0;
  for(auto itr:stats)
  {
    tot_sent += itr.second.txPackets;
  }
  thr <<  curTime << " " << tot_sent << std::endl;
  Simulator::Schedule (Seconds (0.1), &TraceSent, monitor);
}

// calculate metrics
static void
TraceMetrics (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  std::ofstream ofs_thp (exp_name+"/throughput.dat", std::ios::out | std::ios::app);
  std::ofstream ofs_delay (exp_name+"/delay.dat", std::ios::out | std::ios::app);
  std::ofstream ofs_drop (exp_name+"/drop.dat", std::ios::out | std::ios::app);
  std::ofstream ofs_deliver (exp_name+"/delivery.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  
  // threshold
  double tot_thr = 0;
  // delay
  double tot_delay = 0;
  double tot_rx_packets = 0;
  // drop and delivery
  double tot_tx_packets = 0;
  double tot_drop = 0;
  double tot_delivery = 0;
  // total sent
  double tot_sent = 0;
  int num_flows = 0;
  for(auto itr:stats)
  {
    // threshold
    tot_thr += (8 * itr.second.rxBytes ) / (1.0 * curTime.GetSeconds () );
    // delay
    tot_delay += itr.second.delaySum.GetSeconds ();
    tot_rx_packets += itr.second.rxPackets;
    // drop and delivery
    tot_tx_packets += itr.second.txPackets;
    tot_drop += itr.second.lostPackets;
    tot_delivery += itr.second.rxPackets;
    tot_sent += itr.second.txPackets;
    num_flows++;
  }
  ofs_thp <<  curTime << " " << tot_thr << std::endl; // throughput (bit/s)
  ofs_delay <<  curTime << " " << tot_delay/tot_rx_packets << std::endl; // delay (s)
  ofs_drop <<  curTime << " " << (100.0* tot_drop)/tot_tx_packets << std::endl; // drop ratio (%)
  ofs_deliver <<  curTime << " " << (100.0 * tot_delivery)/tot_tx_packets << std::endl; // delivery ratio (%)
  Simulator::Schedule (Seconds (0.1), &TraceMetrics, monitor);
}

////////////////////////////////////////////

bool tracing = true;

uint32_t nWsnNodes;
uint16_t sinkPort=9;
uint32_t mtu_bytes = 180;
uint32_t tcp_adu_size;
uint64_t data_mbytes = 20;
double start_time = 0;
double duration = 100.0;

double stop_time;
bool sack = true;
std::string recovery = "ns3::TcpClassicRecovery";
std::string congestionAlgo = "TcpNewReno";
std::string filePrefix;

Ptr<LrWpanErrorModel>  lrWpanError;
uint16_t nSourceNodes;
void initialize(int argc, char** argv) {

  uint32_t    maxPackets = 100;
  bool        modeBytes  = true;
  double      minTh = 5;
  double      maxTh = 15;
  uint32_t    queueDiscLimitPackets = 300;
  uint32_t    pktSize = 50;

  // std::string bottleNeckLinkBw = "5Mbps";
  // std::string bottleNeckLinkDelay = "50ms";

  // VARIABLE PARAMETERS
  nSourceNodes = 10; // 3 , 6, 10 , 13 , 18 
  std::string appDataRate = "18000bps"; // 3000/50 -> 60, 6000 -> 120, 10000/50 -> 200, 13000/50 -> 260, 18000/50 -> 360
  
  exp_name += "-npan-" + std::to_string(nSourceNodes);
  exp_name += "-app-" + appDataRate;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("tracing", "turn on log components", tracing);
  cmd.AddValue ("nSourceNodes", "turn on log components", nSourceNodes);
  cmd.AddValue ("tracedNode", "turn on log components", tracedNode);
  cmd.Parse (argc, argv);

  if( tracing ) {
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  }

  nWsnNodes = nSourceNodes + 1;
  tracedNode = std::max(1, tracedNode);
  filePrefix = "wpan-sourceCount" + std::to_string(nSourceNodes);

  // Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
  //                     TypeIdValue (TypeId::LookupByName (recovery)));

  // congestionAlgo = "ns3::" + congestionAlgo;
  // Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
  //                     TypeIdValue (TypeId::LookupByName (congestionAlgo)));

  // 2 MB of TCP buffer
  // Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  // Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  // Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

  
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  //////////////////////////////////
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
  // Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
  // Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (pktSize));
  /////////////////////////////////

  // Header* temp_header = new Ipv6Header ();
  // uint32_t ip_header = temp_header->GetSerializedSize ();
  // delete temp_header;
  // temp_header = new TcpHeader ();
  // uint32_t tcp_header = temp_header->GetSerializedSize ();
  // delete temp_header;
  // tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);

  // Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));

  stop_time = start_time + duration;

  lrWpanError = CreateObject<LrWpanErrorModel> ();


  std::cout << "------------------------------------------------------\n"; 
  std::cout << "Source Count: " << nSourceNodes << "\n"; 
  std::cout << "------------------------------------------------------\n"; 
}

int main (int argc, char** argv) {
  initialize(argc, argv);

  Packet::EnablePrinting ();

  NodeContainer wsnNodes;
  wsnNodes.Create (nWsnNodes);

  NodeContainer wiredNodes;
  wiredNodes.Create (1);
  wiredNodes.Add (wsnNodes.Get (0));

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (80),
                                 "DeltaY", DoubleValue (80),
                                 "GridWidth", UintegerValue (10),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wsnNodes);

  LrWpanHelper lrWpanHelper;
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install (wsnNodes);

  lrWpanHelper.AssociateToPan (lrwpanDevices, 0);

  InternetStackHelper internetv6;
  internetv6.Install (wsnNodes);
  internetv6.Install (wiredNodes.Get (0));

  SixLowPanHelper sixLowPanHelper;
  NetDeviceContainer sixLowPanDevices = sixLowPanHelper.Install (lrwpanDevices);

  CsmaHelper csmaHelper;
  NetDeviceContainer csmaDevices = csmaHelper.Install (wiredNodes);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:cafe::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer wiredDeviceInterfaces;
  wiredDeviceInterfaces = ipv6.Assign (csmaDevices);
  wiredDeviceInterfaces.SetForwarding (1, true);
  wiredDeviceInterfaces.SetDefaultRouteInAllNodes (1);

  ipv6.SetBase (Ipv6Address ("2001:f00d::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer wsnDeviceInterfaces;
  wsnDeviceInterfaces = ipv6.Assign (sixLowPanDevices);
  wsnDeviceInterfaces.SetForwarding (0, true);
  wsnDeviceInterfaces.SetDefaultRouteInAllNodes (0);

  for (uint32_t i = 0; i < sixLowPanDevices.GetN (); i++) {
    Ptr<NetDevice> dev = sixLowPanDevices.Get (i);
    dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
    dev->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  // install queue
  // TrafficControlHelper tchBottleneck;
  // QueueDiscContainer queueDiscLeft, queueDiscL;
  // tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
  // queueDiscL = tchBottleneck.Install (wsnNodes.Get (0)->GetDevice(0));

  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
 
  Address sinkLocalAddress (Inet6SocketAddress (Ipv6Address::GetAny (), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApps;
  sinkApps.Add (packetSinkHelper.Install (wiredNodes.Get(1)));
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (90));


  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < nSourceNodes; ++i)
    {
      // Create an on/off app on left side node which sends packets to the right side
      AddressValue remoteAddress (Inet6SocketAddress (wiredDeviceInterfaces.GetAddress(1,1), sinkPort));

      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (wsnNodes.Get(i+1)));
    }
  clientApps.Start (Seconds (2.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (80.0)); // Stop before the sink  

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  std::string dirToSave = "mkdir -p " + exp_name;
  std::string dirToDel = "rm -rf " + exp_name;
  if (system (dirToDel.c_str ()) == -1)
  {
      exit (1);
  }
  if (system (dirToSave.c_str ()) == -1)
  {
      exit (1);
  }


  if (tracing) {
    AsciiTraceHelper ascii;
    lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream (exp_name + "/red-lrwpan-pan.tr"));
    lrWpanHelper.EnablePcapAll (exp_name, false);

    csmaHelper.EnableAsciiAll (ascii.CreateFileStream (exp_name + "/red-lrwpan-csma.tr"));
    csmaHelper.EnablePcapAll (exp_name, false);

    // Simulator::Schedule (Seconds (0.1), &TraceThroughput, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceDelay, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceDrop, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceDelivery, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceSent, flowMonitor);
    Simulator::Schedule (Seconds (0.1), &TraceMetrics, flowMonitor);

  }

  Simulator::Stop (Seconds (100));
  Simulator::Run ();

  flowHelper.SerializeToXmlFile (exp_name + "/flow.xml", true, true);
  // QueueDisc::Stats stLeft = queueDiscL.Get (0)->GetStats ();

  // std::cout << "*** Stats from the bottleneck queue disc (Left) ***" << std::endl;
  // std::cout << stLeft << std::endl;
  
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();

  return 0;
}

