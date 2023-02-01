#include <string>
#include <stdlib.h>
#include <time.h>
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
#include <iostream>

using namespace ns3;

// added from sample.cc and changed
void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
{
  double localThrou = 0;
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
      std::cout <<Simulator::Now ().GetSeconds ()<<std::endl;
      Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
      std::cout << "Flow ID			: "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
      std::cout << "Tx Packets = " << stats->second.txPackets <<'\t'<<'\t';
      std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
      std::cout << "Duration		: "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
      std::cout << stats->second.timeLastRxPacket.GetSeconds () <<'#'<< stats->second.timeFirstTxPacket.GetSeconds () << std::endl;
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


// added from sample.cc and changed
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
      std::cout << "Tx Packets = " << stats->second.txPackets <<'\t'<<'\t';
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

// a load balancing application
class LB : public Application
{
  public:
	// constructor
	LB(uint16_t port, Ipv4InterfaceContainer &right) : mePort(port), meRight(right)
	{
		//seed the (pseudo-)random generator
		std::srand(time(0));
	};
  private:
	//starts the loadbalancing application
	virtual void StartApplication (void)
	{
		// a udp socket to receive incoming data
		meSock = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
		InetSocketAddress local =
                         InetSocketAddress (Ipv4Address::GetAny (), mePort);
		meSock->Bind(local);
		//read the incoming data and pass it on
		meSock->SetRecvCallback(MakeCallback(&LB::readAndPass, this));
		//create tcp sockets to each of the receiving nodes (labeled right in our code)
		for (uint16_t i=0 ; i < meRight.GetN() ; ++i)
		{
			Ptr<Socket> s = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
			InetSocketAddress sAddr =
				 InetSocketAddress (meRight.GetAddress(i), mePort);
			s->Connect(sAddr);
			rightSocks.push_back(s);
		}
	};
	//close the app
	virtual void StopApplication (void)
	{
		//close the sockets
		meSock->Close();
	};
	// the part that recieves and sends data
	void readAndPass(Ptr<Socket> s)
	{
		Ptr<Packet> p;
		Address source;
		uint32_t dest;
		while (p = s->RecvFrom(source))
		{
			//detecs end of file
			if(p->GetSize()==0) break;
			//give destination index a name because we reuse it and it is generated randomly
			dest = std::rand() % meRight.GetN();
			//send
			rightSocks[dest]->Send(p);
			//log
			// NS_LOG_INFO(Simulator::Now().As(Time::S)<<" : packet forwarded to"<<meRight.GetAddress(dest)<<"i on port "<<mePort);
		}
	};


	//private attribiutes
	uint16_t mePort;			// a pirate's port
	Ipv4InterfaceContainer meRight;		// a pirates right hand man (receiver)
	Ptr<Socket> meSock;			// a pirates sock (socket)
	std::vector<Ptr<Socket>> rightSocks;	// a collection of sock(et)s
};


int
main (int argc, char *argv[])
{
  double error = 0.000001;				// error rate
  uint32_t nWifi = 3;					// number of nodes on either side of the load balancer
  std::string phyRate = "HtMcs7";			// Physical layer bitrate

  // Allow users to pick any of the defaults at run-time, via command-line arguments
  CommandLine cmd;
  cmd.AddValue ("error", "Packet error rate", error);
  cmd.AddValue ("nWifi", "Number of wifi STA devices per side", nWifi);

  cmd.Parse (argc, argv);

  // Explicitly create the nodes in the topology.
  // NS_LOG_INFO ("Create nodes");

  NodeContainer left  ;		// senders (on the left of load balancer)
  NodeContainer right ;		// receivers (on the right of load balancer)
  NodeContainer center;		// load balancer (in the center of the diagram)

  left.Create(nWifi);		// create a number of nodes on the left
  right.Create(nWifi);		// create a number of nodes on the right
  center.Create(1);		// create the load balancer

  // NS_LOG_INFO ("Create channels");

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  WifiHelper wifiHelper;
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // set the propagation model
  channel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e8)); 
  YansWifiPhyHelper phy;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n_5GHZ);
  phy.SetChannel (channel .Create ());
  phy.SetErrorRateModel ("ns3::YansErrorRateModel");

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staLeft ;
  NetDeviceContainer staRight;
  NetDeviceContainer apCenter;

  staLeft  = wifi.Install (phy, mac, left  );
  staRight = wifi.Install (phy, mac, right );

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  apCenter = wifi.Install (phy, mac, center);

  //moving senders and receivers
  MobilityHelper mobility;

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (left );
  mobility.Install (right);

  //stationary accesspoint and load balancer
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (center);

  // Create error model on receiver.
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (error));

  // Install the internet stack on the nodes.

  InternetStackHelper stack;
  stack.Install (left  );
  stack.Install (right );
  stack.Install (center);

  Ipv4AddressHelper address;

  // Add IP addresses.
  Ipv4InterfaceContainer wifiInterfaceLeft   ;
  Ipv4InterfaceContainer wifiInterfaceRight  ;
  Ipv4InterfaceContainer wifiInterfaceCenter ;
  address.SetBase ("10.1.3.0", "255.255.255.0");
  wifiInterfaceLeft   = address.Assign (staLeft );
  wifiInterfaceRight  = address.Assign (staRight);
  wifiInterfaceCenter = address.Assign (apCenter);
  
  
  // NS_LOG_INFO ("Create Applications");

  uint16_t port = 25555;

  //we will reuse UDP echo clients as our senders and ignore the responses
  UdpEchoClientHelper echoClient (wifiInterfaceCenter.GetAddress(0), port);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000000000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.01)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (left);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ptr<LB> lbApp = CreateObject<LB> (port, wifiInterfaceRight);
  center.Get (0)->AddApplication (lbApp);
  lbApp->SetStartTime (Seconds (0.0));
  lbApp->SetStopTime (Seconds (10.0));


  //we will reuse TCP sinks as our receivers 
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (right);


  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10));
  
  // NS_LOG_INFO ("Run Simulation");

  std::string fileNameWithNoExtension = "FlowVSThroughput_";
  std::string mainPlotTitle = "Flow vs Throughput";
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

  //one of the two following lines will be commented the other monitors either the throughput or the average endtoend delay

  ThroughputMonitor (&flowHelper, flowMonitor, dataset);
  //AverageDelayMonitor (&flowHelper, flowMonitor, dataset);

  Simulator::Stop (Seconds (11.0));
  Simulator::Run ();
  Simulator::Destroy ();

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

