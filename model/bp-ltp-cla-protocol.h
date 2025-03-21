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
 * Author: Alexander Kedrowitsch <alexk1@vt.edu>
 */
/*
Updates made by: Alexander Kedrowitsch <alexk1@vt.edu>

Aggregate changes for commits in range: ca769ae..f12268c

*/


#ifndef BP_LTP_CLA_PROTOCOL_H
#define BP_LTP_CLA_PROTOCOL_H

#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "bp-cla-protocol.h"
#include "bp-endpoint-id.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "bundle-protocol.h"
#include "bp-routing-protocol.h"
#include "bp-bundle.h"

//#include "ns3/ltp-protocol-helper.h"
#include "ns3/ltp-protocol.h"

#include <map>

namespace ns3 {

class Node;

/**
 * \ingroup bundle-ltp-cla
 *
 * \brief Bundle Protocol LTP convergence layer agent
 *
 * This class implements the Bundle Protocol LTP convergence layer agent.
 * It is a subclass of the abstract base class BpClaProtocol.
 */


class BpLtpClaProtocol : public BpClaProtocol
{
public:
    static TypeId GetTypeId (void);

    /**
     * \brief Constructor
     */
    BpLtpClaProtocol ();
    
    /**
     * \brief Destructor
     */
    virtual ~BpLtpClaProtocol ();

    /**
     * send bundle to the transport layer
     * 
     * \param bundle the bundle to send
    */
    virtual int SendBundle (Ptr<BpBundle> bundle);
    virtual int SendPacket (Ptr<Packet> packet);

    virtual int EnableSend (const BpEndpointId &src, const BpEndpointId &dst);

    /**
     * Enable the transport layer to receive packets
     * 
     * \param local the endpoint id of registration
    */
    virtual int EnableReceive (const BpEndpointId &local);
    virtual int DisableReceive (const BpEndpointId &local);

    virtual void SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol);

    /**
     * Set the CLA protocol
     * 
     * \param protocol pointer to the LTP protocol
    */
    //void SetL4Protocol (ns3::Ptr<ns3::ltp::LtpProtocol> protocol, uint64_t ltpEngineId);
    //virtual void SetL4Protocol (Ptr<Socket> protocol, InetSocketAddress l4address);
    virtual void SetL4Protocol (std::string l4Type);

    int SetL4Address (BpEndpointId eid, uint64_t ltpEngineId);
    uint64_t GetL4Address (BpEndpointId eid, uint64_t returnType);
    virtual InetSocketAddress GetL4Address (BpEndpointId eid, InetSocketAddress returnType);
    virtual int SetL4Address (BpEndpointId eid, const InetSocketAddress* l4Address);

    virtual void SetRoutingProtocol (Ptr<BpRoutingProtocol> route);
    virtual Ptr<BpRoutingProtocol> GetRoutingProtocol (void);
    //int SetL4AddressGeneric (BpEndpointId eid, unsigned char l4Address);
    //unsigned char GetL4AddressGeneric (BpEndpointId eid);
    virtual Ptr<Socket> GetL4Socket (Ptr<Packet> packet);

    void SetL4Callbacks (void);

    void SetLinkStatusCheckDelay (uint32_t delay);

    void SetRedDataMode (int redDataMode);

  /*
  * Red Data modes
  */
    typedef enum {
        RED_DATA_NONE   = 0,
        RED_DATA_SLIM   = 1,
        RED_DATA_ROBUST = 2,
        RED_DATA_ALL    = 3
    } RedDataModes;

private:

  /*
  * Link waiting status
  */
    typedef enum {
        LINK_WAITING_NONE           = 0,
        LINK_WAITING_STATUS_CHECK   = 1,
        LINK_WAITING_LINK_UP_CB     = 2
    } LinkWaitingStatus;

    /**
     * \brief Set callback functions of the transport layer
     * 
    */
    void SetNotificationCallback (void);
    //void SetSendCallback (void);

    void NotificationCallback (ns3::ltp::SessionId id, ns3::ltp::StatusNotificationCode code, std::vector<uint8_t> data, uint32_t dataLength, bool endFlag, uint64_t srcLtpEngine, uint32_t offset);

    void SetLinkStatusChangeCallback (void);
    void LinkStatusChangeCallback ();//(bool linkIsUp);

    std::vector<uint8_t> SplitBundle (Ptr<BpBundle> bundle, RedDataModes redDataMode, uint64_t *redSize);
    Ptr<BpBundle> AssembleBundle (std::vector<uint8_t> data, uint64_t redSize);

    void StartTransmission (Ptr<BpBundle> bundle, BpEndpointId nextHopEid, uint64_t nextHopEngineId, uint64_t redSize);
    void AddBundleToTxQueue (std::vector<uint8_t> cborBundle, uint64_t dstLtpEngineId, uint64_t redSize);
    void SendBundleFromTxQueue (uint64_t dstLtpEngineId);
    void CheckForBundleToSendFromTxQueue(LinkWaitingStatus waitStatus);
    int SendIfLinkUp (uint64_t destLtpEngineId);
    void CheckLinkStatus (void);

    // Callbacks
    void ConnectionRequestSucceededCallback (Ptr<Socket> socket);
    void ConnectionRequestFailedCallback (Ptr<Socket> socket);

    void IncrementTxCnt (void);
    void DecrementTxCnt (void);
    uint32_t GetTxCnt (void);

    struct TxMapVals {
        ns3::ltp::SessionId sessionId;
        ns3::ltp::StatusNotificationCode status;
        BpEndpointId dstEid;
        BpEndpointId srcEid;
        uint64_t srcLtpEngineId;
        uint64_t dstLtpEngineId;
        bool set;
    };

    struct TxQueueVals {
        uint64_t redSize;
        uint64_t dstLtpEngineId;
        std::vector<uint8_t> cborBundle;
        LinkWaitingStatus waitStatus;

    };

    struct RcvMapVals {
        uint64_t srcLtpEngineId;
        ns3::ltp::StatusNotificationCode status;
        std::vector<uint8_t> redData;
        std::vector<uint8_t> greenData;
        uint32_t rcvRedDataLength;
        uint32_t rcvGreenDataLength;
    };

    Ptr<BundleProtocol> m_bp;                                   // bundle protocol
    Ptr<BpRoutingProtocol> m_routing;                           // bundle routing protocol
    std::map<BpEndpointId, uint64_t> m_endpointIdToLtpEngineId; // map endpoint ID to LTP engine ID

    uint64_t m_LtpEngineId;                                     // this node's LTP engine ID
    ns3::Ptr<ns3::ltp::LtpProtocol> m_ltp;                                    // LTP protocol
    Ptr<BpRoutingProtocol> m_bpRouting;                   /// bundle routing protocol

    
    std::map<Ptr<BpBundle>, TxMapVals> m_txSessionMap;            // Map of ongoing Tx sessions
    std::map<ns3::ltp::SessionId, RcvMapVals> m_rcvSessionMap;     // Map of ongoing Rx sessions
    uint32_t m_txCnt;                                            // Counter of all transmissions attempted in current time instance
    //std::queue<TxQueueVals> m_txQueue;                           // Queue of bundles to be transmitted
    std::map<uint64_t, std::deque<TxQueueVals> > m_txQueueMap;    // Map of Tx queues for each LTP engine ID

    // Verifying link availability prior to sending bundle to Ltp
    std::map<Ptr<Socket>, uint64_t> m_socketToLtpEngineId;        // Map of sockets to LTP engine IDs

    uint32_t m_linkStatusCheckDelay;                              // Time delay before rechecking link for status

    RedDataModes m_redDataMode;                                   // Red data mode
};

} // namespace ns3

#endif /* BP_TCP_LTP_PROTOCOL_H */