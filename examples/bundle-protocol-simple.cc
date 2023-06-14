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
/*
void Send (Ptr<BundleProtocol> sender, uint32_t size, BpEndpointId src, BpEndpointId dst)
{
  NS_LOG_INFO ("Send(...) called.");
  std::cout << Simulator::Now ().GetMilliSeconds () << " Send a PDU with size " << size << std::endl;

  Ptr<Packet> packet = Create<Packet> (size);
  sender->Send (packet, src, dst);
}
*/
void Send_char_array (Ptr<BundleProtocol> sender, char* data, BpEndpointId src, BpEndpointId dst)
{
  NS_LOG_INFO ("Sendpacket(...) called.");
  uint32_t size = strlen(data);
  std::cout << Simulator::Now ().GetMilliSeconds () << " Send a PDU with size " << size << ", containing:" << std::endl << data << std::endl;
  //Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*>(data), size);
  //sender->Send_packet (packet, src, dst);
  sender->Send_data (reinterpret_cast<const uint8_t*>(data), size, src, dst);
}
/*
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
*/

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
      //p->CopyData(reinterpret_cast<uint8_t*>(buffer), size);
      buffer[size] = '\0'; // Null terminating char_array to ensure cout doesn't overrun when printing
      std::cout << "Data received: " << std::endl << buffer << std::endl;

      delete [] buffer;
      // Try to get another packet
      data = receiver->Receive_data (eid);
    }
}

void Register (Ptr<BundleProtocol> node, BpEndpointId eid, InetSocketAddress l4Address)
{
    std::cout << Simulator::Now ().GetMilliSeconds () << " Registering external node " << eid.Uri () << std::endl;
    node->ExternalRegister (eid, 0, true, l4Address);
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

  NS_LOG_INFO ("Assign IP Addresses.");  // -- this BP implementation requires IP based networks - issue?
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices); // -- Assuming NS-3 is assinging IPv4 IPs within the specified network to each node in Devices

  NS_LOG_INFO ("Create bundle applications.");
 
 // -- This is getting into CL stuff.  Will need to ensure interface clearly delineates CLAs for each node
  std::ostringstream l4type;
  l4type << "Tcp";
  Config::SetDefault ("ns3::BundleProtocol::L4Type", StringValue (l4type.str ()));
  Config::SetDefault ("ns3::BundleProtocol::BundleSize", UintegerValue (200)); // 400));  // -- is this saying bundles are segmented into 400 bytes?
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (512));  // -- and packets are segmented into 512 bytes (I'm assuming assuming the 112 byte difference is tcp/bp header overhead)

  // build endpoint ids
  BpEndpointId eidSender ("dtn", "node0");
  BpEndpointId eidRecv ("dtn", "node1");

  // get node L4 addresses
  InetSocketAddress Node0Addr (i.GetAddress (0), 9);
  InetSocketAddress Node1Addr (i.GetAddress (1), 9);

  // set bundle static routing for node0
  Ptr<BpStaticRoutingProtocol> RouteNode0 = CreateObject<BpStaticRoutingProtocol> (); // No need for routing when directly connected, just have an object created

  // set bundle static routing for node1
  Ptr<BpStaticRoutingProtocol> RouteNode1 = CreateObject<BpStaticRoutingProtocol> ();  // No need for routing when directly connected, just have an object created

  // sender  
  // -- So each BP node is assigned a routing protocol (with static/dynamic routes)
  // -- Need to ensure seperation from BP routes and CL routes --> look into RFCs!

  // -- are the endpoints being registered here?  How do endpoints get discovered across a network?
  BundleProtocolHelper bpSenderHelper;
  bpSenderHelper.SetRoutingProtocol (RouteNode0);
  bpSenderHelper.SetBpEndpointId (eidSender);
  BundleProtocolContainer bpSenders = bpSenderHelper.Install (nodes.Get (0)); // -- so this is creating an actual instanciation
  bpSenders.Start (Seconds (0.1)); // -- Sender starts at 0.1 sec and stops at 1
  bpSenders.Stop (Seconds (1.0));

  // receiver
  BundleProtocolHelper bpReceiverHelper;
  bpReceiverHelper.SetRoutingProtocol (RouteNode1);
  bpReceiverHelper.SetBpEndpointId (eidRecv);
  BundleProtocolContainer bpReceivers = bpReceiverHelper.Install (nodes.Get (1)); // -- again, the actual instanciation
  bpReceivers.Start (Seconds (0.0)); // -- Receiver starts at 0.0 sec and stops at 1 (assuming to ensure receiver is g2g before sender starts transmitting)
  bpReceivers.Stop (Seconds (1.0));

  // register external nodes with each node
  Simulator::Schedule (Seconds (0.0), &Register, bpSenders.Get (0), eidRecv, Node1Addr);
  Simulator::Schedule (Seconds (0.0), &Register, bpReceivers.Get (0), eidSender, Node0Addr);

  // send 1000 bytes bundle 
  
  //uint32_t size = 1000;  // -- This should be broken into three segments:  400, 400, 200
  //Simulator::Schedule (Seconds (0.2), &Send, bpSenders.Get (0), size, eidSender, eidRecv);  
  // -- above triggers the 'send' function at 0.2 sec mark, sent by the sender, with a specified size, by the sender ID, to the receiver ID
  /* -- Questions:  
    What does the bundle contain?
    Where is pointer to it's contents?
    Where is the application interface?
    Is this the official way to interface with BP or just a simple test?
  */
  // Sending a bundle packet of data
  char data[] = "Books serve to show a man that those original thoughts of his aren't very new after all.";
  
  /*
  char data[] = "Mr. Chairman, this movement is exclusively the work of politicians; "
                "a set of men who have interests aside from the interests of the people, and who, "
                "to say the most of them, are, taken as a mass, at least one long step removed from "
                "honest men. I say this with the greater freedom because, being a politician myself, "
                "none can regard it as personal.";
  */
  /*
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
  */

  NS_LOG_INFO ("Sending data of size: " << strlen(data) << std::endl);
  Simulator::Schedule (Seconds (0.2), &Send_char_array, bpSenders.Get (0), data, eidSender, eidRecv);  

  // receive function
  Simulator::Schedule (Seconds (0.8), &Receive_char_array, bpReceivers.Get (0), eidRecv);

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
