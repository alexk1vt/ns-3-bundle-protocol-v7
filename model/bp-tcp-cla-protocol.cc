/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 University of New Brunswick
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
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */

/*
Updates made by: Alexander Kedrowitsch <alexk1@vt.edu>

Aggregate changes for commits in range: ca769ae..f12268c

Modified/Added Function: GetL4Socket
  - Related commit message: bundle-protocol-multihop-tcp.cc is multi-hopping data between intervening node successfully.  However, BpRouting is not implemented correctly and needs to be updated.  As part of that, node registration needs to advise CLA of next-hop address to keep logic at appropriate levels.
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.

Modified/Added Function: SendPacket
  - Related commit message: bundle-protocol-multihop-tcp.cc is multi-hopping data between intervening node successfully.  However, BpRouting is not implemented correctly and needs to be updated.  As part of that, node registration needs to advise CLA of next-hop address to keep logic at appropriate levels.
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.

Modified/Added Function: EnableSend
  - Related commit message: bundle-protocol-multihop-tcp.cc is multi-hopping data between intervening node successfully.  However, BpRouting is not implemented correctly and needs to be updated.  As part of that, node registration needs to advise CLA of next-hop address to keep logic at appropriate levels.
  - Related commit message: Starting to get Ltp to cooperate. CLA and associated files are still a mess, need cleaning.  Having issue with Ltp protocol deserializing data - had to be sent as uint8_t vector.  Need to investigate
  - Related commit message: Updated BP routing to operate at the BP node level. BP node registration now requires L3/L4 address for CLA to map node to address.  Currently only done withe explicity call to BundleProtocol::ExternalRegister(..)
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.

Modified/Added Function: EnableReceive
  - Related commit message: Updated BP routing to operate at the BP node level. BP node registration now requires L3/L4 address for CLA to map node to address.  Currently only done withe explicity call to BundleProtocol::ExternalRegister(..)
  - Related commit message: Starting to get Ltp to cooperate. CLA and associated files are still a mess, need cleaning.  Having issue with Ltp protocol deserializing data - had to be sent as uint8_t vector.  Need to investigate

Modified/Added Function: DataRecv
  - Related commit message: Updated BP routing to operate at the BP node level. BP node registration now requires L3/L4 address for CLA to map node to address.  Currently only done withe explicity call to BundleProtocol::ExternalRegister(..)
  - Related commit message: Starting to get Ltp to cooperate. CLA and associated files are still a mess, need cleaning.  Having issue with Ltp protocol deserializing data - had to be sent as uint8_t vector.  Need to investigate

Modified/Added Function: DisableReceive
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.

Modified/Added Function: SetL4SocketCallbacks
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.

Modified/Added Function: DataSent
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.

Modified/Added Function: getL4Address
  - Related commit message: Starting to get Ltp to cooperate. CLA and associated files are still a mess, need cleaning.  Having issue with Ltp protocol deserializing data - had to be sent as uint8_t vector.  Need to investigate
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: GetRoutingProtocol
  - Related commit message: Added functionality for CLA to handle links not being available at time of sending.  Included example file to demonstrate.

Modified/Added Function: BpTcpClaProtocol
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: RemoveL4Socket
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: ConnectionSucceeded
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: ConnectionFailed
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: ErrorClose
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: m_send
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: SocketAddressSendQueueEmpty
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.
  - Related commit message: bundle protocol now properly reconstructs fragmented packets.  Havent tested anything larger than 1998 bytes (5 fragmented TCP packets)

Modified/Added Function: SetBundleProtocol
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.

Modified/Added Function: RetrySocketConn
  - Related commit message: have simple example working with cbor encoding. Need to re-implement fragmentation support and clean up commented out code.

*/

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/object-vector.h"

#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"

#include "ns3/config.h" // added by AlexK. to allow modifying default TCP values
#include "ns3/uinteger.h"

#include "bp-tcp-cla-protocol.h"
#include "bp-cla-protocol.h"
#include "bundle-protocol.h"
#include "bp-static-routing-protocol.h"
#include "bp-header.h"
#include "bp-endpoint-id.h"
#include "bp-bundle.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/inet-socket-address.h"

// default port number of dtn bundle tcp convergence layer, which is 
// defined in draft-irtf--dtnrg-tcp-clayer-0.6
#define DTN_BUNDLE_TCP_PORT 4556 

NS_LOG_COMPONENT_DEFINE ("BpTcpClaProtocol");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BpTcpClaProtocol);

TypeId 
BpTcpClaProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BpTcpClaProtocol")
    .SetParent<BpClaProtocol> ()
    .AddConstructor<BpTcpClaProtocol> ()
  ;
  return tid;
}


BpTcpClaProtocol::BpTcpClaProtocol ()
  :m_bp (0),
   m_bpRouting (0)
{ 
  NS_LOG_FUNCTION (this);
  //Added by AlexK.  - Config not within NS3 namespace as documentation stated it would be
  //int tcpSegmentSize = 1000; //to account for bundle fragment sizes exceeding ns-3 default tcp packet size (512)
  //Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
}

BpTcpClaProtocol::~BpTcpClaProtocol ()
{ 
  NS_LOG_FUNCTION (this);
}

void 
BpTcpClaProtocol::SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol)
{ 
  NS_LOG_FUNCTION (this << " " << bundleProtocol);
  m_bp = bundleProtocol;
}

// Does the same as GetL4Socket (Ptr<Packet> packet) but with a bundlePtr
Ptr<Socket>
BpTcpClaProtocol::GetL4Socket (Ptr<BpBundle> bundlePtr)
{
  NS_LOG_FUNCTION (this << " " << bundlePtr);
  BpPrimaryBlock *bpPrimaryBlockPtr = bundlePtr->GetPrimaryBlockPtr ();
  BpEndpointId dst = bpPrimaryBlockPtr->GetDestinationEid ();
  BpEndpointId src = bpPrimaryBlockPtr->GetSourceEid ();

  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4SendSockets.find (src);
  if (it == m_l4SendSockets.end ())
  {
      // enable a tcp connection from the src endpoint id to the dst endpoint id
      if (EnableSend (src, dst) < 0)
        return NULL;

      // update because EnableSende () add new socket into m_l4SendSockets
      it = m_l4SendSockets.find (src);
  }

  return ((*it).second);
}

Ptr<Socket>
BpTcpClaProtocol::GetL4Socket (Ptr<Packet> packet)
{ 
  return NULL;  // AlexK. - this function is not used in the current implementation
  /*
  NS_LOG_FUNCTION (this << " " << packet);
  BpHeader bph;
  packet->PeekHeader (bph);
  BpEndpointId dst = bph.GetDestinationEid ();
  BpEndpointId src = bph.GetSourceEid ();

  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4SendSockets.find (src);
  if (it == m_l4SendSockets.end ())
  {
      // enable a tcp connection from the src endpoint id to the dst endpoint id
      if (EnableSend (src, dst) < 0)
        return NULL;

      // update because EnableSende () add new socket into m_l4SendSockets
      it = m_l4SendSockets.find (src);
  }

  return ((*it).second);
  */
}

bool
BpTcpClaProtocol::RemoveL4Socket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << " " << socket);
  bool status = false;
  for (auto &it : m_l4SendSockets)
  {
    if (it.second == socket)
    {
      m_l4SendSockets.erase (it.first);
      status = true;
      break;
    }
  }
  if (!status)
  {
    return false;
  }
  status = false;
  std::map<Ptr<Socket>, u_int16_t>::iterator iter = m_l4SocketStatus.find (socket);
  if (iter != m_l4SocketStatus.end ())
  {
    m_l4SocketStatus.erase(socket);
    status = true;
  }
  if (!status)
  {
    return false;
  }
  status = false;
  std::map<Ptr<Socket>, InetSocketAddress>::iterator iterate = m_SendSocketL4Addresses.find (socket);
  if (iterate != m_SendSocketL4Addresses.end ())
  {
    m_SendSocketL4Addresses.erase(socket);
    status = true;
  }
  return status;
}

InetSocketAddress
BpTcpClaProtocol::GetSendSocketAddress(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << " " << socket);

  std::map<Ptr<Socket>, InetSocketAddress>::iterator it = m_SendSocketL4Addresses.find (socket);
  if (it == m_SendSocketL4Addresses.end ())
  {
    //entry doesn't exist, so return bad addy
    return InetSocketAddress ("127.0.0.1", 0);
    NS_LOG_FUNCTION (this << " No L4 address recorded for this socket");
  }
  else
  {
    return (*it).second;
  }
}

void
BpTcpClaProtocol::SetSendSocketAddress(Ptr<Socket> socket, InetSocketAddress address)
{
  NS_LOG_FUNCTION (this << " " << socket << " " << address);

  std::map<Ptr<Socket>, InetSocketAddress>::iterator it = m_SendSocketL4Addresses.find (socket);
  if (it == m_SendSocketL4Addresses.end ())
  {
    //entry doesn't exist, so make one
    m_SendSocketL4Addresses.insert(std::pair<Ptr<Socket>, InetSocketAddress>(socket, address));
  }
  else
  {
    // update entry
    (*it).second = address;
  }
  return;
}

/*void
BpTcpClaProtocol::RetrySocketConn (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << " " << packet);
  // redo the SendPacket without interacting with BpSendStore

  Ptr<Socket> socket = GetL4Socket (packet);
  if (socket == NULL)
  {
    NS_LOG_FUNCTION (this << " Unable to create new socket for packet: " << packet);
    return;
  }
}*/

void
BpTcpClaProtocol::RetrySocketConn (Ptr<BpBundle> bundle)
{
  NS_LOG_FUNCTION (this << " " << bundle);
  // redo the SendPacket without interacting with BpSendStore

  Ptr<Socket> socket = GetL4Socket (bundle);
  if (socket == NULL)
  {
    NS_LOG_FUNCTION (this << " Unable to create new socket for packet: " << bundle);
    return;
  }
}

// Does the same as SendPacket (Ptr<Packet> packet) but with a bundlePtr
int
BpTcpClaProtocol::SendBundle (Ptr<BpBundle> bundlePtr)
{
  NS_LOG_FUNCTION (this << " " << bundlePtr);
  Ptr<Socket> socket = GetL4Socket (bundlePtr);

  if ( socket == NULL)
    return -1;

  // retreive bundles from queue in BundleProtocol
  BpPrimaryBlock *bpPrimaryBlockPtr = bundlePtr->GetPrimaryBlockPtr ();
  BpEndpointId src = bpPrimaryBlockPtr->GetSourceEid ();
  BpEndpointId dst = bpPrimaryBlockPtr->GetDestinationEid ();
  Ptr<BpBundle> bundle = m_bp->GetBundle (src); // this is retrieved again here in order to pop the packet from the SendBundleStore! -- TODO: FIX THIS!!

  if (bundle)
    {
      if (GetL4SocketStatus (socket) == 0)
      {
        // tcp session g2g, send immediately
        // Create a packet to place on the wire using CBOR encoded bundle as payload
        uint32_t cborSize = bundle->GetCborEncodingSize ();
        Ptr<Packet> packet = Create<Packet> (reinterpret_cast<uint8_t*>(bundle->GetCborEncoding ()[0]), cborSize);
        BpBundleHeader bundleHeader;
        bundleHeader.SetBundleSize (cborSize);
        packet->AddHeader (bundleHeader);
        NS_LOG_FUNCTION (this << " Sending packet sent from eid: " << src.Uri () << " to eid: " << dst.Uri () << " immediately");
        if (socket->Send (packet) < 0)
        {
          // socket error sending packet
          NS_LOG_FUNCTION (this << " Socket error sending packet");
          return 1;
        }
        return 0;
      }
      else // tcp session not yet ready, store for later sending
      {
        InetSocketAddress address = GetSendSocketAddress (socket);
        // !! TEST FOR BAD ADDRESS
        
        NS_LOG_FUNCTION (this << " BpNode " << m_bp << " with eid " << m_bp->GetBpEndpointId ().Uri () << " Placing packet sent from eid: " << src.Uri () << " to eid: " << dst.Uri () << " into queue for later sending with address: " << address);
        NS_LOG_FUNCTION (this << " checking map at location: " << &SocketAddressSendQueue << "with size of: " << SocketAddressSendQueue.size());

        std::map<InetSocketAddress, std::queue<Ptr<BpBundle> > >::iterator it = SocketAddressSendQueue.find(address);
        if ( it == SocketAddressSendQueue.end ())
        {
          // this is the first packet being sent to this address
          std::queue<Ptr<BpBundle> > qu;
          qu.push (bundle);
          SocketAddressSendQueue.insert (std::pair<InetSocketAddress, std::queue<Ptr<BpBundle> > > (address, qu) );
          NS_LOG_FUNCTION (this << " built new queue for target address.  Queue address: " << &qu);
        }
      else
        {
          // ongoing packet
          (*it).second.push (bundle);
          NS_LOG_FUNCTION (this << " queue now has " << ((*it).second).size () << " items at address: " << &((*it).second));
        }
      }
    }
  NS_LOG_FUNCTION (this << " Unable to get bundle for eid: " << src.Uri ());
  return -1;
}

int
BpTcpClaProtocol::SendPacket (Ptr<Packet> packet)
{ 
  return 0; // AlexK. This is not used anymore, see SendBundle (Ptr<BpBundle> bundlePtr)
  /*
  NS_LOG_FUNCTION (this << " " << packet);
  Ptr<Socket> socket = GetL4Socket (packet);

  if ( socket == NULL)
    return -1;

  // retreive bundles from queue in BundleProtocol
  BpHeader bph;
  packet->PeekHeader (bph);
  BpEndpointId src = bph.GetSourceEid ();
  BpEndpointId dst = bph.GetDestinationEid ();
  Ptr<Packet> pkt = m_bp->GetBundle (src);  // this is retrieved again here in order to pop the packet from the SendBundleStore!
 
  if (pkt)
    {
      if (GetL4SocketStatus (socket) == 0)
      {
        // tcp session g2g, send immediately
        NS_LOG_FUNCTION (this << " Sending packet sent from eid: " << src.Uri () << " to eid: " << dst.Uri () << " immediately");
        if (socket->Send (pkt) < 0)
        {
          // socket error sending packet
          NS_LOG_FUNCTION (this << " Socket error sending packet");
          return 1;
        }
        return 0;
      }
      else // tcp session not yet ready, store for later sending
      {
        InetSocketAddress address = GetSendSocketAddress (socket);
        // !! TEST FOR BAD ADDRESS
        
        NS_LOG_FUNCTION (this << " BpNode " << m_bp << " with eid " << m_bp->GetBpEndpointId ().Uri () << " Placing packet sent from eid: " << src.Uri () << " to eid: " << dst.Uri () << " into queue for later sending with address: " << address);
        NS_LOG_FUNCTION (this << " checking map at location: " << &SocketAddressSendQueue << "with size of: " << SocketAddressSendQueue.size());

        std::map<InetSocketAddress, std::queue<Ptr<Packet> > >::iterator it = SocketAddressSendQueue.find(address);
        if ( it == SocketAddressSendQueue.end ())
        {
          // this is the first packet being sent to this address
          std::queue<Ptr<Packet> > qu;
          qu.push (pkt);
          SocketAddressSendQueue.insert (std::pair<InetSocketAddress, std::queue<Ptr<Packet> > > (address, qu) );
          NS_LOG_FUNCTION (this << " built new queue for target address.  Queue address: " << &qu);
        }
      else
        {
          // ongoing packet
          (*it).second.push (pkt);
          NS_LOG_FUNCTION (this << " queue now has " << ((*it).second).size () << " items at address: " << &((*it).second));
        }
      }
    }
  NS_LOG_FUNCTION (this << " Unable to get bundle for eid: " << src.Uri ());
  return -1;
  */
}

int
BpTcpClaProtocol::EnableReceive (const BpEndpointId &local)
{ 
  NS_LOG_FUNCTION (this << " " << local.Uri ());
  Ptr<BpStaticRoutingProtocol> route = DynamicCast <BpStaticRoutingProtocol> (m_bpRouting);
  //InetSocketAddress addr = route->GetRoute (local);
  BpEndpointId next_hop = route->GetRoute (local);
  InetSocketAddress returnType ("127.0.0.1", 0);
  InetSocketAddress addr = GetL4Address(next_hop, returnType);
  InetSocketAddress badAddr ("1.0.0.1", 0);
  if (addr == badAddr)
    return -1;

  uint16_t port;
  InetSocketAddress defaultAddr ("127.0.0.1", 0);
  if (addr == defaultAddr)
    port = DTN_BUNDLE_TCP_PORT;
  else
    port = addr.GetPort ();

  InetSocketAddress address (Ipv4Address::GetAny (), port);

  // set tcp socket in listen state
  Ptr<Socket> socket = Socket::CreateSocket (m_bp->GetNode (), TcpSocketFactory::GetTypeId ());
  if (socket->Bind (address) < 0)
    return -1;
  if (socket->Listen () < 0)
    return -1;
  if (socket->ShutdownSend () < 0)
    return -1; 

  SetL4SocketCallbacks (socket);
 
  // store the receiving socket so that the convergence layer can dispatch the hundles to different tcp connections
  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4RecvSockets.end ();
  it = m_l4RecvSockets.find (local);
  if (it == m_l4RecvSockets.end ())
    m_l4RecvSockets.insert (std::pair<BpEndpointId, Ptr<Socket> >(local, socket));  
  else
    return -1;


  return 0;
}

int
BpTcpClaProtocol::DisableReceive (const BpEndpointId &local)
{ 
  NS_LOG_FUNCTION (this << " " << local.Uri ());
   std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4RecvSockets.end ();
  it = m_l4RecvSockets.find (local);
  if (it == m_l4RecvSockets.end ())
    {
      return -1;
    }
  else
    {
      // close the tcp conenction
      return ((*it).second)->Close ();
    }

  return 0;
}

int 
BpTcpClaProtocol::EnableSend (const BpEndpointId &src, const BpEndpointId &dst)
{ 
  NS_LOG_FUNCTION (this << " " << src.Uri () << " " << dst.Uri ());
  if (!m_bpRouting)
    NS_FATAL_ERROR ("BpTcpClaProtocol::SendPacket (): cannot find bundle routing protocol");

  // TBD: do not use dynamicast here
  // check route for destination endpoint id
  Ptr<BpStaticRoutingProtocol> route = DynamicCast <BpStaticRoutingProtocol> (m_bpRouting);
  BpEndpointId next_hop = route->GetRoute (dst);
  InetSocketAddress returnType ("127.0.0.1", 0);
  InetSocketAddress address = GetL4Address(next_hop, returnType);

  InetSocketAddress defaultAddr ("127.0.0.1", 0);
  if (address == defaultAddr)
    {
      NS_LOG_DEBUG ("BpTcpClaProtocol::EnableSend (): cannot find route for destination endpoint id " << dst.Uri ());
      return -1;
    }

  // start a tcp connection
  Ptr<Socket> socket = Socket::CreateSocket (m_bp->GetNode (), TcpSocketFactory::GetTypeId ());
  if (socket->Bind () < 0)
  {
    NS_LOG_FUNCTION (this << " " << "Unable to create socket, cannot bind");
    return -1;
  }
  if (socket->Connect (address) < 0)
  {
    NS_LOG_FUNCTION (this << " " << "Unable to create socket, cannot connect to address ");
    return -1;
  }
  NS_LOG_FUNCTION (this << " Requesting connection to address: " << address);
  if (socket->ShutdownRecv () < 0)
  {
    NS_LOG_FUNCTION (this << " " << "Unable to create socket, received shutdown");
    return -1;
  }

  SetL4SocketCallbacks (socket);

  // store the sending socket so that the convergence layer can dispatch the bundles to different tcp connections
  std::map<BpEndpointId, Ptr<Socket> >::iterator it = m_l4SendSockets.find (src);
  if (it == m_l4SendSockets.end ())
    m_l4SendSockets.insert (std::pair<BpEndpointId, Ptr<Socket> >(src, socket));  
  else
    return -1;

  // store initial state of sending socket
  SetL4SocketStatus(socket, 1);

  // store the remote address this socket is connecting to
  SetSendSocketAddress(socket, address);

  return 0;
}

void 
BpTcpClaProtocol::SetL4SocketCallbacks (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  socket->SetConnectCallback (
    MakeCallback (&BpTcpClaProtocol::ConnectionSucceeded, this),
    MakeCallback (&BpTcpClaProtocol::ConnectionFailed, this));

  socket->SetCloseCallbacks (
    MakeCallback (&BpTcpClaProtocol::NormalClose, this),
    MakeCallback (&BpTcpClaProtocol::ErrorClose, this));

  socket->SetAcceptCallback (
    MakeCallback (&BpTcpClaProtocol::ConnectionRequest, this),
    MakeCallback (&BpTcpClaProtocol::NewConnectionCreated, this));

  socket->SetDataSentCallback (
    MakeCallback (&BpTcpClaProtocol::DataSent, this));

  socket->SetSendCallback (
    MakeCallback (&BpTcpClaProtocol::Sent, this));

  socket->SetRecvCallback (
    MakeCallback (&BpTcpClaProtocol::DataRecv, this));
}

void 
BpTcpClaProtocol::ConnectionSucceeded (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " BpNode " << m_bp << " eid " << m_bp->GetBpEndpointId ().Uri ());

  SetL4SocketStatus(socket, 0);

  InetSocketAddress address = GetSendSocketAddress(socket);
  // !! TEST IF GIVEN BAD ADDRESS

  if (!SocketAddressSendQueueEmpty (address))
  {
    // still have packets to send to this address
    NS_LOG_FUNCTION (this << " sending packet to address" << address);
    m_send(socket); // send any waiting packets
  }
} 

void 
BpTcpClaProtocol::ConnectionFailed (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  SetL4SocketStatus(socket, 2);

  InetSocketAddress address = GetSendSocketAddress(socket);
  // !! TEST IF GIVEN BAD ADDRESS
  
  NS_LOG_FUNCTION(this << " was intended for address: " << address);
  std::map<InetSocketAddress, std::queue<Ptr<BpBundle> > >::iterator it = SocketAddressSendQueue.find (address); 
  if ( it == SocketAddressSendQueue.end ())
  {
    NS_LOG_FUNCTION (this << " Did not find packet in SocketAddressSendQueue for address: " << address);
    return;
  }

  if ( ((*it).second).size () == 0)
  {
    NS_LOG_FUNCTION (this << " Found queue in SocketAddressSendQueue for address: " << address << " but is empty");
    return;
  }
  Ptr<BpBundle> bundle = ((*it).second).front ();
  // retrieved packet from socket send queue, now ok to remove records of socket and close
  NS_LOG_FUNCTION (this << " packets remaining to send, clearing out old socket and attempting resend");
  if (!RemoveL4Socket (socket))
  {
    NS_LOG_FUNCTION (this << " unable to remove socket from records");
  }

  //socket->Close(); // Socket already being closed by ns-3; errors-out if attempt to manually close
  
  // socket records removed, now try to resend
  //ResendPacket(packet);
  RetrySocketConn (bundle);
}

void 
BpTcpClaProtocol::NormalClose (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  SetL4SocketStatus(socket, 3);
}

void 
BpTcpClaProtocol::ErrorClose (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  SetL4SocketStatus(socket, 4);

  InetSocketAddress address = GetSendSocketAddress (socket);
  // !! TEST FOR BAD ADDRESS
  if (!SocketAddressSendQueueEmpty (address))
  {
    // still have packets to send to this address
    NS_LOG_FUNCTION (this << " still have remaining packets to send to address" << address);
  }
  if (!RemoveL4Socket (socket))
  {
    NS_LOG_FUNCTION (this << " unable to remove socket from records");
  }
}

bool
BpTcpClaProtocol::ConnectionRequest (Ptr<Socket> socket, const Address &address)
{ 
  NS_LOG_FUNCTION (this << " " << socket);// << " " << address);
  return true;
}

void 
BpTcpClaProtocol::NewConnectionCreated (Ptr<Socket> socket, const Address &address)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " " << address << " BpNode " << m_bp << " eid " << m_bp->GetBpEndpointId ().Uri ());
  SetL4SocketCallbacks (socket);  // reset the callbacks due to fork in TcpSocketBase
}

void 
BpTcpClaProtocol::DataSent (Ptr<Socket> socket, uint32_t size)
{ 
  NS_LOG_FUNCTION (this << " " << socket << " " << size);
}

void 
BpTcpClaProtocol::Sent (Ptr<Socket> socket, uint32_t size)
{ 
  BpEndpointId eid = m_bp->GetBpEndpointId ();
  NS_LOG_FUNCTION (this << " Socket:" << socket << " Size:" << size << " From node uri: " << eid.Uri ());
}


void 
BpTcpClaProtocol::DataRecv (Ptr<Socket> socket)
{ 
  NS_LOG_FUNCTION (this << " " << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
   {
     m_bp->ReceivePacket (packet);
   }
}

int
BpTcpClaProtocol::SetL4Address (BpEndpointId eid, const InetSocketAddress* l4Address)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri() << " " << l4Address->GetIpv4());
  
  std::map<BpEndpointId, InetSocketAddress>::iterator it = m_l4Addresses.find (eid);
  if (it == m_l4Addresses.end ())
    m_l4Addresses.insert (std::pair<BpEndpointId, InetSocketAddress>(eid, *l4Address));  
  else
    return -1;
  
  return 0;
}
/*
int
BpTcpClaProtocol::setL4AddressLtp (BpEndpointId eid, uint64_t l4Address)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri () << " " << l4address);

  return -1;
}
*/
InetSocketAddress
BpTcpClaProtocol::GetL4Address (BpEndpointId eid, InetSocketAddress returnType)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri());

  std::map<BpEndpointId, InetSocketAddress>::iterator it = m_l4Addresses.find (eid);
  if (it == m_l4Addresses.end ())
  {
    InetSocketAddress badAddr ("1.0.0.1", 0);
    return badAddr;
  }
  else
    return ((*it).second); 
}
/*
uint64_t
BpTcpClaProtocol::getL4AddressLtp (BpEndpointId eid)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri());

  InetSocketAddress badAddr ("1.0.0.1", 0);
  return badAddr;
}
*/
/*
void*
BpTcpClaProtocol::getL4Address (BpEndpointId eid)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri());

  InetSocketAddress l4SocketAddress = getL4AddressTcp (eid);
  return reinterpret_cast<void*>(&l4SocketAddress);
  //return reinterpret_cast<void*>(l4SocketAddress.GetLocal().GetPtr());
}
*/
/*
int
BpTcpClaProtocol::setL4AddressGeneric (BpEndpointId eid, unsigned char &l4Address)
{
  InetSocketAddress l4SocketAddress = reinterpret_cast<InetSocketAddress>(l4Address);
  NS_LOG_FUNCTION (this << " " << eid.Uri() << " " << l4Address.GetIpv4());
  
  std::map<BpEndpointId, InetSocketAddress>::iterator it = m_l4Addresses.find (eid);
  if (it == m_l4Addresses.end ())
    m_l4Addresses.insert (std::pair<BpEndpointId, InetSocketAddress>(eid, l4Address));  
  else
    return -1;
  
  return 0;
}
*/

/*
unsigned char
BpTcpClaProtocol::getL4AddressGeneric (BpEndpointId eid)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri());

  std::map<BpEndpointId, InetSocketAddress>::iterator it = m_l4Addresses.find (eid);
  if (it == m_l4Addresses.end ())
  {
    InetSocketAddress badAddr ("1.0.0.1", 0);
    unsigned char badAddrGeneric = reinterpret_cast<unsigned char>(badAddr);
    return badAddr;
  }
  else
    unsigned char addressGeneric = reinterpret_cast<unsigned char>((*it).second);
    return addressGeneric;
}
*/

void
BpTcpClaProtocol::SetL4Protocol (std::string l4Type)
{
  NS_LOG_FUNCTION (this << " " << l4Type);
}

void 
BpTcpClaProtocol::SetL4SocketStatus (Ptr<Socket> socket, u_int16_t status)
{
  NS_LOG_FUNCTION (this << " " << socket << " " << status);

  std::map<Ptr<Socket>, u_int16_t>::iterator it = m_l4SocketStatus.find (socket);
  if (it == m_l4SocketStatus.end ())
  {
    m_l4SocketStatus.insert(std::pair<Ptr<Socket>, u_int16_t>(socket, status));
  }
  else
  {
    (*it).second = status;
  }
  return;
}

u_int16_t
BpTcpClaProtocol::GetL4SocketStatus (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << " " << socket);

  std::map<Ptr<Socket>, u_int16_t>::iterator it = m_l4SocketStatus.find (socket);
  if (it == m_l4SocketStatus.end ())
  {
    return 5;
  }
  else
  {
    return ((*it).second);
  }
}

void
BpTcpClaProtocol::SetRoutingProtocol (Ptr<BpRoutingProtocol> route)
{ 
  NS_LOG_FUNCTION (this << " " << route);
  m_bpRouting = route;
}

Ptr<BpRoutingProtocol>
BpTcpClaProtocol::GetRoutingProtocol ()
{ 
  NS_LOG_FUNCTION (this);
  return m_bpRouting;
}

void
BpTcpClaProtocol::m_send (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << " " << socket);

  InetSocketAddress address = GetSendSocketAddress (socket);
  // !! TEST FOR BAD ADDRESS

  while (true)
  {
    std::map<InetSocketAddress, std::queue<Ptr<BpBundle> > >::iterator it = SocketAddressSendQueue.find (address); 
    if ( it == SocketAddressSendQueue.end ())
    {
      NS_LOG_FUNCTION (this << " Did not find packet in SocketAddressSendQueue for address: " << address);
      return;
    }
    else
    {
      if ( ((*it).second).size () == 0)
      {
        NS_LOG_FUNCTION (this << " Found queue in SocketAddressSendQueue for address: " << address << " but is empty");
        return;
      }
      Ptr<BpBundle> bundle = ((*it).second).front ();
      ((*it).second).pop (); // remove packet from queue
      NS_LOG_FUNCTION (this << " Sending packet to address: " << address << " immediately");
      // Create packet to send on the wire with bundle cbor encoding as payload
      std::vector <std::uint8_t> cborEncodedBundle = bundle->GetCborEncoding ();
      uint32_t cborSize = bundle->GetCborEncodingSize ();
      NS_LOG_FUNCTION (this << " Sending bundle with cborSize: " << cborSize);
      //uint32_t elementCount = cborEncodedBundle.size();
      //uint32_t calculatedSize = elementCount * sizeof(uint8_t);
      //NS_LOG_FUNCTION (this << " Recalcuated cbor size: " << calculatedSize);
      //Ptr<Packet> packet = Create<Packet> (reinterpret_cast<uint8_t*>(&cborEncodedBundle[0]), cborSize);
      Ptr<Packet> packet = Create<Packet> (reinterpret_cast<uint8_t*>(&cborEncodedBundle[0]), cborSize);
      BpBundleHeader bundleHeader;
      bundleHeader.SetBundleSize (cborSize);
      packet->AddHeader (bundleHeader);
      NS_LOG_FUNCTION (this << " Sending packet with size: " << packet->GetSize () << " header reports: " << bundleHeader);
      // *** Testing ***
      /*
      // Make a copy of the packet
      Ptr<Packet> testPacket = Create<Packet> (reinterpret_cast<uint8_t*>(&cborEncodedBundle[0]), cborSize);
      testPacket->AddHeader (bundleHeader);
      // now strip packet down like it's being received
      BpBundleHeader testBundleHeader;
      packet->PeekHeader (testBundleHeader);
      uint32_t testCborBundleSize = testBundleHeader.GetBundleSize ();
      packet->RemoveHeader (testBundleHeader);
      NS_LOG_FUNCTION (this << "  ::Testing:: packet header reports CBOR bundle size of: " << testCborBundleSize);
      uint8_t testBuffer[testCborBundleSize];
      packet->CopyData (testBuffer, testCborBundleSize);
      std::vector <uint8_t> testV_buffer (testBuffer, testBuffer + testCborBundleSize);
      CompareCborBytesToVector(cborEncodedBundle, testV_buffer);
      NS_LOG_FUNCTION (this <<" cborEncoded Bundle dump:");
      PrintCborBytes(cborEncodedBundle);
      NS_LOG_FUNCTION (this << " testV_buffer dump:");
      PrintCborBytes (testV_buffer);
      //Ptr<BpBundle> testBundle = Create<BpBundle> ();
      //testBundle->SetBundleFromCbor (bundle->GetCborEncoding ());
      //NS_LOG_FUNCTION (this << " Testing: bundle cbor bytes:");
      //testBundle->PrintCborBytes ();
      */
      // *** End Testing ***
      if (socket->Send (packet) < 0)
      {
        // socket error sending packet
        NS_LOG_FUNCTION (this << " Socket error sending packet");
      }
    }
  }
}

bool
BpTcpClaProtocol::SocketAddressSendQueueEmpty (InetSocketAddress address)
{
  NS_LOG_FUNCTION (this << " " << address << " checking map at location: " << &SocketAddressSendQueue << "with size of: " << SocketAddressSendQueue.size());

  std::map<InetSocketAddress, std::queue<Ptr<BpBundle> > >::iterator it = SocketAddressSendQueue.find (address); 
  if ( it == SocketAddressSendQueue.end ())
  {
    NS_LOG_FUNCTION (this << " Did not find packet in SocketAddressSendQueue for address: " << address);
    return true;
  }
  else
  {
    if ( ((*it).second).size () == 0)
    {
      NS_LOG_FUNCTION (this << " Found queue in SocketAddressSendQueue for address: " << address << " but is empty");
      return true;
    }
    NS_LOG_FUNCTION (this << " Found queue in SocketAddressSendQueue for address: " << address << " with this number of packets: " << ((*it).second).size () << " at address: " << &((*it).second));
    return false;
  }
}

} // namespace ns3
