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

*/

#ifndef BP_TCP_CLA_PROTOCOL_H
#define BP_TCP_CLA_PROTOCOL_H

#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "bp-cla-protocol.h"
#include "bp-endpoint-id.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "bundle-protocol.h"
#include "bp-routing-protocol.h"
#include "bp-bundle.h"
#include <map>

namespace ns3 {

class Node;
//class Socket;
//class BpSocket;

class BpTcpClaProtocol : public BpClaProtocol
{
public:

  static TypeId GetTypeId (void);

  /**
   * \brief Constructor
   */
  BpTcpClaProtocol ();

  /**
   * Destroy
   */
  virtual ~BpTcpClaProtocol ();

  /**
   * send packet to the transport layer
   *
   * \param packet packet to sent
   */
  virtual int SendPacket (Ptr<Packet> packet);

  /**
   * send bundle to the transport layer
   * 
   * \param bundle bundle to send
   */
  virtual int SendBundle (Ptr<BpBundle> bundle);

  /**
   * Set the TCP socket in listen state;
   *
   * \param local the local endpoint id
   */
  virtual int EnableReceive (const BpEndpointId &local);

  /**
   * Close the TCP connection
   *
   * \param local the endpoint id of registration
   */
  virtual int DisableReceive (const BpEndpointId &local);


  /**
   * Enable this bundle node to send bundles at the transport layer
   * This method calls the bind (), connect primitives of tcp socket
   *
   * \param src the source endpoint id
   * \param dst the destination endpoint id
   */
  virtual int EnableSend (const BpEndpointId &src, const BpEndpointId &dst);


  /**
   * \brief Get the transport layer socket
   *
   * This method finds the socket by the source endpoint id of the bundle. 
   * If it cannot find, which means that this bundle is the first bundle required
   * to be transmitted for this source endpoint id, it start a tcp connection with
   * the destination endpoint id.
   *
   * \param packet the bundle required to be transmitted
   *
   * \return return NULL if it cannot find a socket for the source endpoing id of 
   * the bunle. Otherwise, it returns the socket.
   */
  virtual Ptr<Socket> GetL4Socket (Ptr<Packet> packet);

  /**
   * \brief Get the transport layer socket when given a bundle
   * 
   * \param bundle the bundle required to be transmitted
  */
  Ptr<Socket> GetL4Socket (Ptr<BpBundle> bundle);

  
  virtual bool RemoveL4Socket (Ptr<Socket> socket);

  /**
   * Connect to routing protocol
   *
   * \param route routing protocol
   */
  void SetRoutingProtocol (Ptr<BpRoutingProtocol> route);

  /**
   * Get routing protocol
   *
   * \return routing protocol
   */
  virtual Ptr<BpRoutingProtocol> GetRoutingProtocol ();

  /**
   * Connect to bundle protocol
   *
   * \param bundleProtocol bundle protocol
   */
  void SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol);

  /**
   *  Callbacks methods callded by NotifyXXX methods in TcpSocketBase
   *
   *  Those methods will further call the callback wrap methods in BpSocket
   *  The only exception is the DataRecv (), which will call BpSocket::Receive ()
   *
   *  The operations within those methods will be updated in the furture versions
   */

  /**
   * \brief connection succeed callback
   */
  void ConnectionSucceeded (Ptr<Socket> socket );

  /**
   * \brief connection fail callback
   */
  void ConnectionFailed (Ptr<Socket> socket);

  /**
   * \brief normal close callback
   */
  void NormalClose (Ptr<Socket> socket);

  /**
   * \brief error close callback
   */
  void ErrorClose (Ptr<Socket> socket);

  /**
   * \brief connection succeed callback
   */
  bool ConnectionRequest (Ptr<Socket>,const Address &);

  /**
   * \brief new connection created callback
   */
  void NewConnectionCreated (Ptr<Socket>, const Address &);

  /**
   * \brief data sent callback
   */
  void DataSent (Ptr<Socket>,uint32_t size);

  /**
   * \brief sent callback
   */
  void Sent (Ptr<Socket>,uint32_t size);

  /**
   * \brief data receive callback
   */
  void DataRecv (Ptr<Socket> socket);

  virtual int SetL4Address (BpEndpointId eid, const InetSocketAddress* l4Address);
  //virtual int setL4AddressLtp (BpEndpointId eid, InetSocketAddress l4Address) = 0;
  //virtual int setL4Address (BpEndpointId eid, void* l4Address);
  virtual InetSocketAddress GetL4Address (BpEndpointId eid, InetSocketAddress returnType);
  //uint64_t getL4AddressLtp (BpEndpointId eid) = 0;
  //virtual void* getL4Address (BpEndpointId eid);
  //virtual void SetL4Protocol (Ptr<Socket> protocol, InetSocketAddress address);
  virtual void SetL4Protocol (std::string l4Type);

  //virtual int setL4AddressGeneric (BpEndpointId eid, unsigned char &l4Address);
  //virtual unsigned char* getL4AddressGeneric (BpEndpointId eid);
  

private:

  /**
   * Set callbacks of the transport layer
   *
   * \param socket the transport layer socket
   */
  virtual void SetL4SocketCallbacks (Ptr<Socket> socket);

  virtual void SetL4SocketStatus (Ptr<Socket> socket, u_int16_t status);
  virtual u_int16_t GetL4SocketStatus (Ptr<Socket> socket);
  virtual InetSocketAddress GetSendSocketAddress(Ptr<Socket> socket);
  virtual void SetSendSocketAddress(Ptr<Socket> socket, InetSocketAddress address);

  virtual void m_send (Ptr<Socket> socket);
  virtual bool SocketAddressSendQueueEmpty (InetSocketAddress address);

  //virtual void ResendPacket (Ptr<Packet> packet);

  virtual void RetrySocketConn (Ptr<BpBundle> packet);

private:

  Ptr<BundleProtocol> m_bp;                             /// bundle protocol
  std::map<BpEndpointId, Ptr<Socket> > m_l4SendSockets; /// the transport layer sender sockets
  std::map<BpEndpointId, Ptr<Socket> > m_l4RecvSockets; /// the transport layer receiver sockets
  std::map<BpEndpointId, InetSocketAddress> m_l4Addresses; /// the registered node socket addresses
  std::map<Ptr<Socket>, u_int16_t> m_l4SocketStatus; // the status of registered sockets:  ok to send _only_ when status is 0
                                                    // 0 - New connection created, ok to send
                                                    // 1 - Connection request sent, waiting for response
                                                    // 2 - Connection attempt failed, not ok to send
                                                    // 3 - Connection closed normally
                                                    // 4 - Connection closed by error
                                                    // 5 - No status
  std::map<Ptr<Socket>, InetSocketAddress> m_SendSocketL4Addresses; // map of destination addresses to corresponding sockets (since you cant query sockets for the remote address they are connected to)
  std::map<InetSocketAddress, std::queue<Ptr<BpBundle> > > SocketAddressSendQueue; // storage of packets going to a particular L4 address while waiting for TCP sessions to be built
  Ptr<BpRoutingProtocol> m_bpRouting;                   /// bundle routing protocol
};

} // namespace ns3

#endif /* BP_TCP_CLA_PROTOCOL_H */

