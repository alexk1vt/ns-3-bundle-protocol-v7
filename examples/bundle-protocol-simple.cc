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

// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using bundle protocol.
// - Tracing of queues and packet receptions to file "bundle-protocol-simple.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/bp-endpoint-id.h"
#include "ns3/bundle-protocol.h"
#include "ns3/bp-static-routing-protocol.h"
#include "ns3/bundle-protocol-helper.h"
#include "ns3/bundle-protocol-container.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BundleProtocolSimpleExample");

void Send (Ptr<BundleProtocol> sender, uint32_t size, BpEndpointId src, BpEndpointId dst)
{
  NS_LOG_INFO ("Send(...) called.");
  std::cout << Simulator::Now ().GetMilliSeconds () << " Send a PDU with size " << size << std::endl;

  Ptr<Packet> packet = Create<Packet> (size);
  sender->Send (packet, src, dst);
}

void Receive (Ptr<BundleProtocol> receiver, BpEndpointId eid)
{

  Ptr<Packet> p = receiver->Receive (eid);
  NS_LOG_INFO ("Receive(..) called.");
  while (p != NULL)
    {
      std::cout << Simulator::Now ().GetMilliSeconds () << " Receive bundle size " << p->GetSize () << std::endl;
      p = receiver->Receive (eid);
    }
}

int
main (int argc, char *argv[])
{

  bool tracing = true;

  ns3::PacketMetadata::Enable ();

  NS_LOG_INFO ("Create bundle nodes.");
  NodeContainer nodes;
  nodes.Create (2);

  NS_LOG_INFO ("Create channels.");

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer devices; // -- NS-3 stuff, need to understand
  devices = pointToPoint.Install (nodes);

  InternetStackHelper internet; // -- More NS-3 stuff, also need to understand
  internet.Install (nodes);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices); // -- Assuming NS-3 is assinging IPv4 IPs within the specified network to each node in Devices

  NS_LOG_INFO ("Create bundle applications.");
 
 // -- This is getting into CL stuff.  Will need to ensure interface clearly delineates CLAs for each node
  std::ostringstream l4type;
  l4type << "Tcp";
  Config::SetDefault ("ns3::BundleProtocol::L4Type", StringValue (l4type.str ()));
  Config::SetDefault ("ns3::BundleProtocol::BundleSize", UintegerValue (400));  // -- is this saying bundles are segmented into 400 bytes?
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (512));  // -- and packets are segmented into 512 bytes (I'm assuming assuming the 112 byte difference is tcp/bp header overhead)

  // build endpoint ids
  BpEndpointId eidSender ("dtn", "node0");
  BpEndpointId eidRecv ("dtn", "node1");

  // set bundle static routing
  Ptr<BpStaticRoutingProtocol> route = CreateObject<BpStaticRoutingProtocol> ();
  route->AddRoute (eidSender, InetSocketAddress (i.GetAddress (0), 9));
  route->AddRoute (eidRecv, InetSocketAddress (i.GetAddress (1), 9));

  // sender  
  // -- So each BP node is assigned a routing protocol (with static/dynamic routes)
  // -- Need to ensure seperation from BP routes and CL routes --> look into RFCs!
  BundleProtocolHelper bpSenderHelper;
  bpSenderHelper.SetRoutingProtocol (route);
  bpSenderHelper.SetBpEndpointId (eidSender);
  BundleProtocolContainer bpSenders = bpSenderHelper.Install (nodes.Get (0)); // -- so this is creating an actual instanciation
  bpSenders.Start (Seconds (0.1)); // -- Sender starts at 0.1 sec and stops at 1
  bpSenders.Stop (Seconds (1.0));

  // receiver
  BundleProtocolHelper bpReceiverHelper;
  bpReceiverHelper.SetRoutingProtocol (route);
  bpReceiverHelper.SetBpEndpointId (eidRecv);
  BundleProtocolContainer bpReceivers = bpReceiverHelper.Install (nodes.Get (1)); // -- again, the actual instanciation
  bpReceivers.Start (Seconds (0.0)); // -- Receiver starts at 0.0 sec and stops at 1 (assuming to ensure receiver is g2g before sender starts transmitting)
  bpReceivers.Stop (Seconds (1.0));

  // send 1000 bytes bundle 
  uint32_t size = 1000;  // -- This should be broken into three segments:  400, 400, 200
  Simulator::Schedule (Seconds (0.2), &Send, bpSenders.Get (0), size, eidSender, eidRecv);  
  // -- above triggers the 'send' function at 0.2 sec mark, sent by the sender, with a specified size, by the sender ID, to the receiver ID
  /* -- Questions:  
    What does the bundle contain?
    Where is pointer to it's contents?
    Where is the application interface?
    Is this the official way to interface with BP or just a simple test?
  */
  // receive function
  Simulator::Schedule (Seconds (0.8), &Receive, bpReceivers.Get (0), eidRecv);

  if (tracing)
    {
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("bundle-protocol-simple.tr"));
      pointToPoint.EnablePcapAll ("bundle-protocol-simple", false);
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (1.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

}
