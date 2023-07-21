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

#include "ns3/ltp-protocol-helper.h"
#include "ns3/ltp-protocol.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BundleProtocolMultihopLtpExample");

void Send_char_array (Ptr<BundleProtocol> sender, char* data, BpEndpointId src, BpEndpointId dst)
{
  NS_LOG_INFO ("Sendpacket(...) called.");
  uint32_t size = strlen(data);
  std::cout << Simulator::Now ().GetMilliSeconds () << " Send a PDU with size " << size << ", containing:" << std::endl << data << std::endl;
  sender->Send_data (reinterpret_cast<const uint8_t*>(data), size, src, dst);
}

void Receive_char_array (Ptr<BundleProtocol> receiver, BpEndpointId eid)
{

  //Ptr<Packet> p = receiver->Receive (eid);
  std::vector<uint8_t> data = receiver->Receive_data (eid);
  NS_LOG_INFO ("Receive(..) called.");
  while (!data.empty())
    {
      uint32_t size = data.size();
      std::cout << Simulator::Now ().GetMilliSeconds () << " Receive bundle size " << size << std::endl;
      char* buffer = new char[size+1];
      std::copy(data.begin(), data.end(), buffer);
      buffer[size] = '\0'; // Null terminating char_array to ensure cout doesn't overrun when printing
      std::cout << "Data received: " << std::endl << buffer << std::endl;

      delete [] buffer;
      // Try to get another packet
      data = receiver->Receive_data (eid);
    }
}

void Register (Ptr<BundleProtocol> node, BpEndpointId eid, uint64_t l4Address)
{
    std::cout << Simulator::Now ().GetMilliSeconds () << " Registering external node " << eid.Uri () << std::endl;
    node->ExternalRegisterLtp (eid, 0, true, l4Address);
}

int
main (int argc, char *argv[])
{

  bool tracing = true;

  ns3::PacketMetadata::Enable ();

  NS_LOG_INFO ("Create bundle nodes.");
  NodeContainer nodes, link1_nodes, link2_nodes;
  nodes.Create (3);

  link1_nodes.Add(nodes.Get(0));
  link1_nodes.Add(nodes.Get(1));

  link2_nodes.Add(nodes.Get(1));
  link2_nodes.Add(nodes.Get(2));
  

  NS_LOG_INFO ("Create channels.");

  InternetStackHelper internet;
  Ipv4ListRoutingHelper routingList;
  Ipv4StaticRoutingHelper staticRoutingHelper;
  routingList.Add(staticRoutingHelper, 0);

  internet.SetRoutingHelper(routingList);
  internet.Install (nodes);

  std::ostringstream channelDelay;
  channelDelay << "20ms";
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (channelDelay.str ()));

  NetDeviceContainer link1_devices, link2_devices;
  link1_devices = pointToPoint.Install (link1_nodes);
  link2_devices = pointToPoint.Install (link2_nodes);


  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer link1_i = ipv4.Assign (link1_devices);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer link2_i = ipv4.Assign (link2_devices);

  Ptr<Ipv4> ipv4_node0 = link1_nodes.Get(0)->GetObject<Ipv4> ();
  Ptr<Ipv4StaticRouting> staticRouting_node0 = staticRoutingHelper.GetStaticRouting (ipv4_node0);

  staticRouting_node0->AddNetworkRouteTo (Ipv4Address ("0.0.0.0"), Ipv4Mask ("0.0.0.0"), 1); // setting node0 interface 1 as default route (interface 0 is loopback)

  Ptr<Ipv4> ipv4_node2 = link2_nodes.Get(1)->GetObject<Ipv4> ();
  Ptr<Ipv4StaticRouting> staticRouting_node2 = staticRoutingHelper.GetStaticRouting (ipv4_node2);

  staticRouting_node2->AddNetworkRouteTo (Ipv4Address ("0.0.0.0"), Ipv4Mask ("0.0.0.0"), 1);  // setting node0 interface 1 as default route (interface 0 is loopback)

  NS_LOG_INFO ("Create bundle applications.");
  // Now install LTP on the nodes

  // Defines the ClientService ID code that will be using the Ltp protocol in both sides of the link
  // Bundle protocol code as defined by IANA: "LTP Client Service Identifiers" referenced in RFC 7116
  uint64_t ClientServiceId = 1; // 1 -- bundle protocol service id

  // Creta a LtpIpResolution table to perform mappings between Ipv4 adresses and LtpEngineIDs
  Ptr<ns3::ltp::LtpIpResolutionTable> routing =  CreateObjectWithAttributes<ns3::ltp::LtpIpResolutionTable> ("Addressing", StringValue ("Ipv4"));

  // Use a helper to create and install Ltp Protocol instances in the nodes.
  
  ns3::ltp::LtpProtocolHelper ltpHelper;
  ltpHelper.SetAttributes ("CheckPointRtxLimit",  UintegerValue (20),
                           "ReportSegmentRtxLimit", UintegerValue (20),
                           "RetransCyclelimit",  UintegerValue (20),
                           "OneWayLightTime", StringValue (channelDelay.str ()));
  ltpHelper.SetLtpIpResolutionTable (routing);
  ltpHelper.SetBaseLtpEngineId (0);
  //ltpHelper.SetStartTransmissionTime (Seconds (1));
  
  ltpHelper.InstallAndLink (nodes);

  // get node L4 addresses
  uint64_t senderL4addr = link1_nodes.Get (0)->GetObject<ns3::ltp::LtpProtocol> ()->GetLocalEngineId ();
  uint64_t forwarderL4addr_link1 = link1_nodes.Get (1)->GetObject<ns3::ltp::LtpProtocol> ()->GetLocalEngineId ();
  uint64_t forwarderL4addr_link2 = link2_nodes.Get (0)->GetObject<ns3::ltp::LtpProtocol> ()->GetLocalEngineId ();
  uint64_t recvL4addr = link2_nodes.Get (1)->GetObject<ns3::ltp::LtpProtocol> ()->GetLocalEngineId ();
  // ltpHelper sets remote peers in round-robin fashion
  // So, need to have forwarder linked back to sender and receiver linked back to forwarder
  ltpHelper.Link (link1_nodes.Get (1), senderL4addr);
  ltpHelper.Link (link2_nodes.Get (1), forwarderL4addr_link2);

  // Callback registration will occur within the bundle protocol CLA initiatization
  // Configure the BP nodes for LTP
  std::ostringstream l4type;
  l4type << "Ltp";
  Config::SetDefault ("ns3::BundleProtocol::L4Type", StringValue (l4type.str ()));
  Config::SetDefault ("ns3::BundleProtocol::BundleSize", UintegerValue (400));  // set bundle fragmentation size to 400 bytes

  // build endpoint ids
  BpEndpointId eidSender ("dtn", "node0");
  BpEndpointId eidForwarder ("dtn", "node1");
  BpEndpointId eidRecv ("dtn", "node2");
 
  // set bundle static routing for sender
  Ptr<BpStaticRoutingProtocol> route_sender = CreateObject<BpStaticRoutingProtocol> (); // static routes for sender
  route_sender->AddRoute (eidRecv, eidForwarder); // dest: recv; next_hop: forwarder

  // set bundle static routing for forwarder
  Ptr<BpStaticRoutingProtocol> route_forwarder = CreateObject<BpStaticRoutingProtocol> (); // static routes for forwarder

  // set bundle static routing for recv
  Ptr<BpStaticRoutingProtocol> route_recv = CreateObject<BpStaticRoutingProtocol> (); // static routes for recv
  route_recv->AddRoute (eidSender, eidForwarder); // dest: sender; next_hop: forwarder

  // sender  
  BundleProtocolHelper bpSenderHelper;
  //bpSenderHelper.SetRoutingProtocol (route);
  bpSenderHelper.SetRoutingProtocol (route_sender);
  bpSenderHelper.SetBpEndpointId (eidSender);
  BundleProtocolContainer bpSenders = bpSenderHelper.Install (link1_nodes.Get (0));
  bpSenders.Start (Seconds (0.2));
  bpSenders.Stop (Seconds (10.0));

  // forwarder
  BundleProtocolHelper bpForwarderHelper;
  //bpForwarderHelper.SetRoutingProtocol (route);
  bpForwarderHelper.SetRoutingProtocol (route_forwarder);
  bpForwarderHelper.SetBpEndpointId (eidForwarder);
  BundleProtocolContainer bpForwarders = bpForwarderHelper.Install (link1_nodes.Get (1));
  bpForwarders.Start (Seconds (0.1));
  bpForwarders.Stop (Seconds (10.0));

  // receiver
  BundleProtocolHelper bpReceiverHelper;
  //bpReceiverHelper.SetRoutingProtocol (route);
  bpReceiverHelper.SetRoutingProtocol (route_recv);
  bpReceiverHelper.SetBpEndpointId (eidRecv);
  BundleProtocolContainer bpReceivers = bpReceiverHelper.Install (link2_nodes.Get (1));
  bpReceivers.Start (Seconds (0.0));
  bpReceivers.Stop (Seconds (10.0));

  // register external nodes with each node
  Simulator::Schedule (Seconds (0.0), &Register, bpSenders.Get (0), eidForwarder, forwarderL4addr_link1);
  Simulator::Schedule (Seconds (0.0), &Register, bpSenders.Get (0), eidRecv, recvL4addr);

  Simulator::Schedule (Seconds (0.0), &Register, bpForwarders.Get (0), eidSender, senderL4addr);
  Simulator::Schedule (Seconds (0.0), &Register, bpForwarders.Get (0), eidRecv, recvL4addr);

  Simulator::Schedule (Seconds (0.0), &Register, bpReceivers.Get (0), eidForwarder, forwarderL4addr_link2);
  Simulator::Schedule (Seconds (0.0), &Register, bpReceivers.Get (0), eidSender, senderL4addr);

  /*
  char data[] = "Mr. Chairman, this movement is exclusively the work of politicians; "
                "a set of men who have interests aside from the interests of the people, and who, "
                "to say the most of them, are, taken as a mass, at least one long step removed from "
                "honest men. I say this with the greater freedom because, being a politician myself, "
                "none can regard it as personal.";
  */  
  char data[] = "The Senate of the United States shall be composed of two Senators from each State, "
                "chosen by the Legislature thereof, for six Years; and each Senator shall have one Vote."
                "Immediately after they shall be assembled in Consequence of the first Election, they shall"
                " be divided as equally as may be into three Classes. The Seats of the Senators of the"
                " first Class shall be vacated at the Expiration of the second Year, of the second Class at"
                " the Expiration of the fourth Year, and of the third Class at the Expiration of the sixth"
                " Year, so that one third may be chosen every second Year; and if Vacancies happen by "
                "Resignation, or otherwise, during the Recess of the Legislature of any State, the Executive"
                " thereof may make temporary Appointments until the next Meeting of the Legislature, which"
                " shall then fill such Vacancies. No Person shall be a Senator who shall not have attained"
                " to the Age of thirty Years, and been nine Years a Citizen of the United States, and who"
                " shall not, when elected, be an Inhabitant of that State for which he shall be chosen."
                "The Vice President of the United States shall be President of the Senate, but shall have "
                "no Vote, unless they be equally divided.  The Senate shall chuse their other Officers, "
                "and also a President pro tempore, in the Absence of the Vice President, or when he shall"
                " exercise the Office of President of the United States. The Senate shall have the sole "
                "Power to try all Impeachments. When sitting for that Purpose, they shall be on Oath or"
                " Affirmation. When the President of the United States is tried, the Chief Justice shall"
                " preside: And no Person shall be convicted without the Concurrence of two thirds of the "
                "Members present. Judgment in Cases of Impeachment shall not extend further than to removal"
                " from Office, and disqualification to hold and enjoy any Office of honor, Trust or Profit"
                " under the United States: but the Party convicted shall nevertheless be liable and subject"
                " to Indictment, Trial, Judgment and Punishment, according to Law.";
  

  // sending data bundle
  NS_LOG_INFO ("Sending data of size: " << strlen(data) << std::endl);
  Simulator::Schedule (Seconds (0.3), &Send_char_array, bpSenders.Get (0), data, eidSender, eidRecv);  

  // receive function
  Simulator::Schedule (Seconds (8), &Receive_char_array, bpReceivers.Get (0), eidRecv);
  if (false)
    {
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("bundle-protocol-multihop-tcp.tr"));
      pointToPoint.EnablePcapAll ("bundle-protocol-multihop-tcp", false);
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

}
