
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/ngpsr-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
//#include "ns3/common-module.h"
//#include "ns3/node-module.h"
//#include "ns3/helper-module.h"
//#include "ns3/mobility-module.h"
//#include "ns3/contrib-module.h"
#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"
#include "ns3/udp-echo-helper.h"
#include <iostream>
#include <cmath>

using namespace ns3;

class NGpsrExample
{
public:
  NGpsrExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t size;
  /// Width of the Node Grid
  uint32_t gridWidth;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  //\}

  ///\name network
  //\{
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  NGpsrExample test;
  if (! test.Configure(argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
NGpsrExample::NGpsrExample () :
  // Number of Nodes
  size (100),
  // Grid Width
  gridWidth(10),
  // Distance between nodes
  step (100), //TODO Distance changed to the limit between nodes: test to see if there are transmitions
  // Simulation time
  totalTime (30),
  // Generate capture files for each node
  pcap (false)
{
}

bool
NGpsrExample::Configure (int argc, char **argv)
{
  // Enable NGPSR logs by default. Comment this if too noisy
  //LogComponentEnable("NGpsrRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed(12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  return true;
}

void
NGpsrExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  NGpsrHelper ngpsr;
  ngpsr.Set ("LocationServiceName", StringValue ("GOD"));
  ngpsr.Install ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
NGpsrExample::Report (std::ostream &)
{
}

void
NGpsrExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
     {
       std::ostringstream os;
       os << "node-" << i;
       Names::Add (os.str (), nodes.Get (i));
     }
  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0.0),
                                "MinY", DoubleValue (0.0),
                                "DeltaX", DoubleValue (step),
                                "DeltaY", DoubleValue (step),
                                "GridWidth", UintegerValue (gridWidth),
                                "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
}

void
NGpsrExample::CreateDevices ()
{
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("ngpsr"));
    }
}

void
NGpsrExample::InstallInternetStack ()
{
  NGpsrHelper ngpsr;
  // you can configure NGPSR attributes here using ngpsr.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (ngpsr);
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces = address.Assign (devices);
}

void
NGpsrExample::InstallApplications ()
{

  uint16_t port = 9;  // well-known echo port number
  uint32_t packetSize = 1024; // size of the packets being transmitted
  uint32_t maxPacketCount = 100; // number of packets to transmit
  Time interPacketInterval = Seconds (1.); // interval between packet transmissions

  // Set-up  a server Application on the bottom-right node of the grid
  UdpEchoServerHelper server1 (port);
  uint16_t server1Position = size-1; //bottom right
  ApplicationContainer apps = server1.Install (nodes.Get(server1Position));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (totalTime-0.1));

  // Set-up a client Application, connected to 'server1', to be run on the top-left node of the grid
  UdpEchoClientHelper client1 (interfaces.GetAddress (server1Position), port);
  client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client1.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client1.SetAttribute ("PacketSize", UintegerValue (packetSize));
  uint16_t client1Position = 0; //top left
  apps = client1.Install (nodes.Get (client1Position));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (totalTime-0.1));

  // Set-up a server Application on the top-right node of the grid
  UdpEchoServerHelper server2 (port);
  uint16_t server2Position = 9;
  apps = server2.Install (nodes.Get(server2Position)); //top right
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (totalTime-0.1));

  // Set-up a client Application, connected to 'server2', to be run on the bottom-left node of the grid
  UdpEchoClientHelper client2 (interfaces.GetAddress (server2Position), port);
  client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  uint16_t client2Position = size-9; //bottom left
  apps = client2.Install (nodes.Get (client2Position));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (totalTime-0.1));

}
