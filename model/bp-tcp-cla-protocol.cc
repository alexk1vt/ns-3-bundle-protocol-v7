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
#include "ns3/tcp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"

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

Ptr<Socket>
BpTcpClaProtocol::GetL4Socket (Ptr<Packet> packet)
{ 
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

void
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
}

int
BpTcpClaProtocol::SendPacket (Ptr<Packet> packet)
{ 
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
}

int
BpTcpClaProtocol::EnableReceive (const BpEndpointId &local)
{ 
  NS_LOG_FUNCTION (this << " " << local.Uri ());
  Ptr<BpStaticRoutingProtocol> route = DynamicCast <BpStaticRoutingProtocol> (m_bpRouting);
  //InetSocketAddress addr = route->GetRoute (local);
  BpEndpointId next_hop = route->GetRoute (local);
  InetSocketAddress addr = getL4Address(next_hop);
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
 
  // store the sending socket so that the convergence layer can dispatch the hundles to different tcp connections
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
  InetSocketAddress address = getL4Address(next_hop);

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
  std::map<InetSocketAddress, std::queue<Ptr<Packet> > >::iterator it = SocketAddressSendQueue.find (address); 
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
  Ptr<Packet> packet = ((*it).second).front ();
  // retrieved packet from socket send queue, now ok to remove records of socket and close
  NS_LOG_FUNCTION (this << " packets remaining to send, clearing out old socket and attempting resend");
  if (!RemoveL4Socket (socket))
  {
    NS_LOG_FUNCTION (this << " unable to remove socket from records");
  }

  //socket->Close(); // Socket already being closed by ns-3; errors-out if attempt to manually close
  
  // socket records removed, now try to resend
  //ResendPacket(packet);
  RetrySocketConn (packet);
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
BpTcpClaProtocol::setL4Address (BpEndpointId eid, InetSocketAddress l4Address)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri() << " " << l4Address.GetIpv4());
  
  std::map<BpEndpointId, InetSocketAddress>::iterator it = m_l4Addresses.find (eid);
  if (it == m_l4Addresses.end ())
    m_l4Addresses.insert (std::pair<BpEndpointId, InetSocketAddress>(eid, l4Address));  
  else
    return -1;
  
  return 0;
}

InetSocketAddress
BpTcpClaProtocol::getL4Address (BpEndpointId eid)
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
    std::map<InetSocketAddress, std::queue<Ptr<Packet> > >::iterator it = SocketAddressSendQueue.find (address); 
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
      Ptr<Packet> packet = ((*it).second).front ();
      ((*it).second).pop (); // remove packet from queue
      NS_LOG_FUNCTION (this << " Sending packet to address: " << address << " immediately");
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

  std::map<InetSocketAddress, std::queue<Ptr<Packet> > >::iterator it = SocketAddressSendQueue.find (address); 
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
