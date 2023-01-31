//       Network topology
//
//       n0 -------------- n1
//            1Mbps-10ms

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-error-rate-model.h"
#include "ns3/ssid.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpComparision");

Ptr<PacketSink> tcpSink;

void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
{
  double localThrou = 0;
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
      Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
      std::cout << "Flow ID			: "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
      std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
      std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
      std::cout << "Duration		: "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
      std::cout << "Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
      std::cout << "Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024  << " Mbps" << std::endl;
      localThrou = stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024;
      if (stats->first == 1)
        {
          DataSet.Add ((double)Simulator::Now ().GetSeconds (),(double) localThrou);
        }
      std::cout << "---------------------------------------------------------------------------" << std::endl;
    }
  Simulator::Schedule (Seconds (10),&ThroughputMonitor, fmhelper, flowMon,DataSet);
  flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
}

void
AverageDelayMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
{
  double localDelay = 0;
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
      Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
      std::cout << "Flow ID			: "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
      std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
      std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
      std::cout << "Duration		: "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
      std::cout << "Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
      std::cout << "Sum of e2e Delay: " << stats->second.delaySum.GetSeconds () << " s" << std::endl;
      std::cout << "Average of e2e Delay: " << stats->second.delaySum.GetSeconds () / stats->second.rxPackets << " s" << std::endl;
      localDelay = stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024;
      if (stats->first == 1)
        {
          DataSet.Add ((double)Simulator::Now ().GetSeconds (),(double) localDelay);
        }
      std::cout << "---------------------------------------------------------------------------" << std::endl;
    }
  Simulator::Schedule (Seconds (10),&AverageDelayMonitor, fmhelper, flowMon,DataSet);
  flowMon->SerializeToXmlFile ("AverageDelayMonitor.xml", true, true);
}


int
main (int argc, char *argv[])
{
  uint32_t maxBytes = 0;
  double error = 0.000001;
  double duration = 50.0;
  bool verbose = true;
  uint32_t nWifi = 7;
  bool tracing = false;

  // Allow users to pick any of the defaults at run-time, via command-line arguments
  CommandLine cmd;
  cmd.AddValue ("maxBytes", "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("error", "Packet error rate", error);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc, argv);

  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
  
  // Explicitly create the nodes in the topology.
  NS_LOG_INFO ("Create nodes");

  NodeContainer nodes;
  nodes.Create (2);

  NS_LOG_INFO ("Create channels");

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = wifiStaNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  phy.SetErrorRateModel ("ns3::YansErrorRateModel");

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  Ipv4InterfaceContainer wifiInterfaces ;
  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  wifiInterfaces = address.Assign (apDevices);

//  UdpEchoServerHelper echoServer (9);
//
//  //ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (nWifi-2));
//  ApplicationContainer serverApps = echoServer.Install (wifiApNode );
//  serverApps.Start (Seconds (1.0));
//  serverApps.Stop (Seconds (10.0));
//
//  UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (0), 9);
//  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
//  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
//  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
//
//  ApplicationContainer clientApps = 
//    echoClient.Install (wifiApNode );
//  clientApps.Start (Seconds (2.0));
//  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10.0));

  if (tracing)
    {
      phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      phy.EnablePcap ("third", apDevices.Get (0));
    }

  Simulator::Run ();
  // Create error model on receiver.
//  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
//  em->SetAttribute ("ErrorRate", DoubleValue (error));
//  apDevices.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  //wifiApNode.Get(0) ->SetAttribute("ErrorRateModel",DoubleValue(0.001));

  // Install the internet stack on the nodes.
  //InternetStackHelper internet;
  //internet.Install (nodes);
//
  // Add IP addresses.
//  NS_LOG_INFO ("Assign IP Addresses");
//  Ipv4AddressHelper ipv4;
//  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
//  Ipv4InterfaceContainer ipv4Container = ipv4.Assign (devices);

  NS_LOG_INFO ("Create Applications");

//  // Create a BulkSendApplication and install it on node 0.
  uint16_t port = 1102;
//
  BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (wifiInterfaces.GetAddress (2), port));
  // Set the amount of data to send in bytes. Zero is unlimited.
  //source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (wifiStaNodes.Get (0));

  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (duration));

  // Create a PacketSinkApplication and install it on node 1.
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (wifiStaNodes.Get (2));


  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (duration));

  // Just for debug (in our code).
  tcpSink = DynamicCast<PacketSink> (sinkApps.Get (0));

  NS_LOG_INFO ("Run Simulation");

  std::string fileNameWithNoExtension = "errorVSThroughput_";
  std::string mainPlotTitle = "Error vs Throughput";
  std::string graphicsFileName        = fileNameWithNoExtension + ".png";
  std::string plotFileName            = fileNameWithNoExtension + ".plt";
  std::string plotTitle               = mainPlotTitle + ", Error: " + std::to_string(error);
  std::string dataTitle               = "Throughput";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot (graphicsFileName);
  gnuplot.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot.SetLegend ("Flow", "Throughput");


  Gnuplot2dDataset dataset;
  dataset.SetTitle (dataTitle);
  dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  // Flow monitor.
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll ();

  //ThroughputMonitor (&flowHelper, flowMonitor, dataset);
  AverageDelayMonitor (&flowHelper, flowMonitor, dataset);

  Simulator::Stop (Seconds (duration));
  Simulator::Run ();

  //Gnuplot ...continued.
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str ());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();
  return 0;
}
