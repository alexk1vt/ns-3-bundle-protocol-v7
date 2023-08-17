/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bp-ltp-cla-protocol.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/object-vector.h"
#include "ns3/packet.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"


#include "ns3/node.h"
#include "ns3/simulator.h"

#include "ns3/uinteger.h"

#include "bp-cla-protocol.h"
#include "bundle-protocol.h"
#include "bp-static-routing-protocol.h"
#include "bp-header.h"
#include "bp-endpoint-id.h"
#include "bp-bundle.h"
//#include "ns3/ltp-protocol-helper.h"
#include "ns3/ltp-protocol.h"
//#include "ns3/ltp-header.h"

// default port number of dtn bundle udp convergence layer, defined in RFC 7122
// section 3.3.2 for LTP using UDP transport
#define DTN_BUNDLE_LTP_PORT 1113

NS_LOG_COMPONENT_DEFINE ("BpLtpClaProtocol");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BpLtpClaProtocol);

TypeId
BpLtpClaProtocol::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::BpLtpClaProtocol")
        .SetParent<BpClaProtocol> ()
        .AddConstructor<BpLtpClaProtocol> ();
    return tid;
}

BpLtpClaProtocol::BpLtpClaProtocol ()
    : m_bp (0),
      m_bpRouting (0),
      m_txCnt (0),
      m_linkStatusCheckDelay (1),
      m_redDataMode (RED_DATA_SLIM)
{
    NS_LOG_FUNCTION (this);
}

BpLtpClaProtocol::~BpLtpClaProtocol ()
{
    NS_LOG_FUNCTION (this);

    m_bp = 0;
    m_routing = 0;
    m_ltp = 0;
    m_bpRouting = 0;

    m_endpointIdToLtpEngineId.clear ();
    m_txSessionMap.clear ();
    m_rcvSessionMap.clear ();
}

void
BpLtpClaProtocol::SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol)
{
    NS_LOG_FUNCTION (this << bundleProtocol);
    m_bp = bundleProtocol;
}

void
BpLtpClaProtocol::SendBundleToNextHop (Ptr<BpBundle> bundle, BpEndpointId nextHopEid)
{
    NS_LOG_FUNCTION (this << bundle << nextHopEid.Uri () );

    BpPrimaryBlock *bpPrimaryBlockPtr = bundle->GetPrimaryBlockPtr ();
    BpEndpointId src = bpPrimaryBlockPtr->GetSourceEid ();

    // Send the bundle
    // Create mapping to keep track of bundle and its ongoing corresponding Ltp Session
    // Mapping may need to wait until LtpSession is created
    // Look up destination Engine Id based on destEID

    BpEndpointId internalEid = m_bp->GetBpEndpointId ();
    
    uint64_t returnType {(uint64_t) -1};
    uint64_t nextHopEngineId = GetL4Address (nextHopEid, returnType);
    if (nextHopEngineId == (uint64_t) -1)
    {
        NS_LOG_FUNCTION (this << " Unable to get L4 address for eid: " << nextHopEid.Uri ());
        return;
    }
    
    //RedDataModes redDataMode = RED_DATA_SLIM; // Options are RED_DATA_NONE, RED_DATA_SLIM, RED_DATA_ROBUST, RED_DATA_ALL
    //uint64_t redSize = 0;
    
    // store bundle pointer in Tx Session Map for later updating and tracking _prior_ to sending
    TxMapVals txMapVals;
    txMapVals.dstEid = nextHopEid;
    txMapVals.srcEid = internalEid;
    txMapVals.srcLtpEngineId = m_LtpEngineId;
    txMapVals.dstLtpEngineId = nextHopEngineId;
    txMapVals.set = false;
    std::map<Ptr<BpBundle>, TxMapVals>::iterator it = m_txSessionMap.find (bundle);
    if (it == m_txSessionMap.end ())
    {
        m_txSessionMap.insert (std::pair<Ptr<BpBundle>, TxMapVals>(bundle, txMapVals));
        NS_LOG_FUNCTION (this << " Inserted bundle into txSessionMap with srcLtpEngineId: " << m_LtpEngineId);
    }
    else
    {
        NS_LOG_FUNCTION (this << " Unable to insert bundle into txSessionMap");
        return;
    }

    NS_LOG_FUNCTION (this << " Internal engine id: " << m_LtpEngineId << " Queuing bundle from eid: " << src.Uri () << " to nextHopEid: " << nextHopEid.Uri () << " with nextHopEngineId: " << nextHopEngineId << " to send.");
    // if redDataMode == RED_DATA_SLIM or RED_DATA_ROBUST, need to split bundle into red/green parts
    // if redDataMode == RED_DATA_NONE or RED_DATA_ALL, send entire bundle as either green or part
    if (m_redDataMode == RED_DATA_NONE)
    {
        AddBundleToTxQueue (bundle->GetCborEncoding (), nextHopEngineId, 0);
    }
    else if (m_redDataMode == RED_DATA_ALL)
    {
        AddBundleToTxQueue (bundle->GetCborEncoding (), nextHopEngineId, bundle->GetCborEncodingSize ());
    }
    else
    {
        uint64_t redSize = 0;
        std::vector<uint8_t> splitCborBundle = SplitBundle (bundle, m_redDataMode, &redSize);
        AddBundleToTxQueue (splitCborBundle, nextHopEngineId, redSize);
    }

    return;
}

int
BpLtpClaProtocol::SendBundle (Ptr<BpBundle> bundlePtr)
{
    NS_LOG_FUNCTION (this << " " << bundlePtr);
    BpPrimaryBlock *bpPrimaryBlockPtr = bundlePtr->GetPrimaryBlockPtr ();
    BpEndpointId src = bpPrimaryBlockPtr->GetSourceEid ();
    
    Ptr<BpBundle> bundle = m_bp->GetBundle (src); // this is retrieved again here in order to pop the packet from the SendBundleStore! -- TODO: FIX THIS!!

    if (bundle)
    {
        bpPrimaryBlockPtr = bundle->GetPrimaryBlockPtr ();
        BpEndpointId dst = bpPrimaryBlockPtr->GetDestinationEid ();
        Ptr<BpStaticRoutingProtocol> route = DynamicCast<BpStaticRoutingProtocol> (m_bpRouting);
        if (!route->HasAlternateRoute (dst))
        {
            
            NS_LOG_FUNCTION (this << " No alternate route exists for eid: " << dst.Uri ());
            BpEndpointId nextHopEid = route->GetRoute (dst);
            SendBundleToNextHop (bundle,  nextHopEid);
            return 0;
        }

        NS_LOG_FUNCTION (this << " Alternate route exists for eid: " << dst.Uri ());
        BpEndpointId primaryNextHopEid = route->GetRoute (dst);
        BpEndpointId alternateNextHopEid = route->GetAlternateRoute (dst);
        
        // Build sockets to test the connection with
        TypeId tid;
        tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

        Ptr<Socket> alternateSocket = Socket::CreateSocket (m_bp->GetNode (), tid);
        if (alternateSocket->Bind () == 0)
        {
            alternateSocket->SetConnectCallback(MakeCallback (&BpLtpClaProtocol::L4LinkToEidConnectionRequestSucceededCallback, this),
                                                MakeCallback (&BpLtpClaProtocol::L4LinkToEidConnectionRequestFailedCallback, this));
        }
        else
        {
            NS_LOG_FUNCTION (this << " Unable to bind socket to alternate route; defaulting to primary route");
            SendBundleToNextHop (bundle,  primaryNextHopEid);
            return 0;
        }

        Ptr<Socket> primarySocket = Socket::CreateSocket (m_bp->GetNode (), tid);
        if (primarySocket->Bind () != 0)
        {
            NS_LOG_FUNCTION (this << " Unable to bind socket to primary route");
            return -1;
        }
        primarySocket->SetConnectCallback(MakeCallback (&BpLtpClaProtocol::L4LinkToEidConnectionRequestSucceededCallback, this),
                                                MakeCallback (&BpLtpClaProtocol::L4LinkToEidConnectionRequestFailedCallback, this));
        
        // Build struct to insert into vector
        TxLinkCheckVectorVals txLinkCheckVectorVals;
        txLinkCheckVectorVals.bundle = bundle;
        txLinkCheckVectorVals.primarySocket = primarySocket;
        txLinkCheckVectorVals.primaryDstEid = primaryNextHopEid;
        txLinkCheckVectorVals.primaryRouteStatus = 0;
        txLinkCheckVectorVals.alternateSocket = alternateSocket;
        txLinkCheckVectorVals.alternateDstEid = alternateNextHopEid;
        txLinkCheckVectorVals.alternateRouteStatus = 0;
        
        m_txLinkStatusVector.push_back (txLinkCheckVectorVals);

        // Test altenate route first
        if (CheckL4LinkToEid (alternateNextHopEid, alternateSocket) < 0)
        {
            // initial check to alternate route failed; manually performing failure callback
            L4LinkToEidConnectionRequestFailedCallback (alternateSocket);
            // initial check to alternate route failed; defaulting to primary
            //alternateSocket->Close ();
            //primarySocket->Close ();
            //m_txLinkStatusVector.pop_back ();
            //SendBundleToNextHop (bundle,  primaryNextHopEid);
            //return 0;
        }
        // Test primary route
        if (CheckL4LinkToEid (primaryNextHopEid, primarySocket) < 0)
        {
            // initial check to alternate route failed; manually performing failure callback
            L4LinkToEidConnectionRequestFailedCallback (primarySocket);
            // initial check to primary route failed; using alternate
            //alternateSocket->Close ();
            //primarySocket->Close ();
            //m_txLinkStatusVector.pop_back ();
            //SendBundleToNextHop (bundle,  alternateNextHopEid);
            //return 0;
        }
        return 0;
    }
    NS_LOG_FUNCTION (this << " Unable to get bundle for eid: " << src.Uri ());
    return -1;
}

void
BpLtpClaProtocol::SetRedDataMode (int redDataMode)
{
    NS_LOG_FUNCTION (this << redDataMode);
    m_redDataMode = static_cast<RedDataModes>(redDataMode);
}

void
BpLtpClaProtocol::IncrementTxCnt (void)
{
    NS_LOG_FUNCTION (this << " m_txCnt: " << m_txCnt);
    m_txCnt++;
}

void
BpLtpClaProtocol::DecrementTxCnt (void)
{
    NS_LOG_FUNCTION (this << " m_txCnt: " << m_txCnt);
    m_txCnt--;
    if (m_txCnt == 0)
    {
        m_txCnt = 0;
    }
}

uint32_t
BpLtpClaProtocol::GetTxCnt (void)
{
    NS_LOG_FUNCTION (this << " m_txCnt: " << m_txCnt);
    return m_txCnt;
}

void
BpLtpClaProtocol::StartTransmission (Ptr<BpBundle> bundle, BpEndpointId nextHopEid, uint64_t nextHopEngineId, uint64_t redSize)
{
    NS_LOG_FUNCTION (this << " bundle: " << bundle << " nextHopEid: " << nextHopEid.Uri () << " nextHopEngineId: " << nextHopEngineId << " redSize: " << redSize);
    BpEndpointId internalEid = m_bp->GetBpEndpointId ();
    // uint32_t cborSize = bundle->GetCborEncodingSize ();
    std::vector <uint8_t> cborEncoding = bundle->GetCborEncoding ();
    //uint8_t* heapBuffer = new uint8_t[cborSize];
    //std::copy(cborEncoding.begin(), cborEncoding.end(), heapBuffer);
    //std::vector <uint8_t> cborEncoding (cborSize, 65); // Red segment works for a vector of just '65s' but not for a vector of actual data...
    
    uint64_t ClientServiceId = 1; // 1 - bundle protocol

    NS_LOG_FUNCTION (this << " Internal engine id: " << m_LtpEngineId << " Sending packet sent from eid: " << internalEid.Uri () << " to nextHopEid: " << nextHopEid.Uri () << " with nextHopEngineId: " << nextHopEngineId << " of size " << cborEncoding.size() << " bytes immediately");
    m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, nextHopEngineId, cborEncoding, redSize);
    //m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, nextHopEngineId, heapBuffer, redSize);
}

void
BpLtpClaProtocol::AddBundleToTxQueue (std::vector<uint8_t> cborBundle, uint64_t dstLtpEngineId, uint64_t redSize)
{
    NS_LOG_FUNCTION (this << " dstLtpEngineId: " << dstLtpEngineId << " redSize: " << redSize);
    TxQueueVals txQueueVals;
    txQueueVals.redSize = redSize;
    txQueueVals.dstLtpEngineId = dstLtpEngineId;
    txQueueVals.cborBundle = cborBundle;
    
    std::map<uint64_t, std::deque<TxQueueVals> >::iterator it = m_txQueueMap.find (dstLtpEngineId);
    if (it == m_txQueueMap.end ())
    {
        // queue for this dstLtpEngineId doesn't exist yet, so create it
        txQueueVals.waitStatus = LINK_WAITING_STATUS_CHECK; // will always wait on initial request to send
        std::deque<TxQueueVals> txQueue;
        txQueue.push_back (txQueueVals);
        m_txQueueMap.insert (std::pair<uint64_t, std::deque<TxQueueVals> >(dstLtpEngineId, txQueue));
        // Since queue was just created, send the bundle immediately
        //SendBundleFromTxQueue (dstLtpEngineId);
        if (SendIfLinkUp (dstLtpEngineId) < 0)
        {
            NS_LOG_FUNCTION (this << " Unable to make connection request to destination address: " << dstLtpEngineId << ". Scheduling link status check");
            Simulator::Schedule (Seconds (m_linkStatusCheckDelay), &BpLtpClaProtocol::CheckLinkStatus, this);
        }
    }
    else
    {
        // queue for this dstLtpEngineId already exists, so add to it
        if (it->second.size () == 0)
        {
            txQueueVals.waitStatus = LINK_WAITING_STATUS_CHECK; // will always wait on initial request to send
            it->second.push_back (txQueueVals);
            NS_LOG_FUNCTION (this << " TxQueueVals size is 1, sending immediately");
            //Simulator::ScheduleNow (&BpLtpClaProtocol::SendBundleFromTxQueue, this);
            //SendBundleFromTxQueue (dstLtpEngineId);
            if (SendIfLinkUp (dstLtpEngineId) < 0)
            {
                NS_LOG_FUNCTION (this << " Unable to make connection request to destination address: " << dstLtpEngineId << ". Scheduling link status check");
                Simulator::Schedule (Seconds (m_linkStatusCheckDelay), &BpLtpClaProtocol::CheckLinkStatus, this);
            }
        }
        else
        {
            txQueueVals.waitStatus = LINK_WAITING_NONE;
            it->second.push_back (txQueueVals);
            NS_LOG_FUNCTION (this << " Bundles already waiting in queue to send");            
        }
    }
}

void
BpLtpClaProtocol::CheckLinkStatus (void)
{
    NS_LOG_FUNCTION (this);

    // Try sending any bundles that are waiting on link status check
    CheckForBundleToSendFromTxQueue (LINK_WAITING_LINK_UP_CB);

    // See if any bundles are still waiting and schedule another check if so
    std::map<uint64_t, std::deque<TxQueueVals> >::iterator it = m_txQueueMap.begin ();
    while (it != m_txQueueMap.end ())
    {
        if (it->second.size () > 0)
        {
            if (it->second.front ().waitStatus == LINK_WAITING_STATUS_CHECK)
            {
                NS_LOG_FUNCTION (this << " Bundles still waiting for link status check, scheduling another check");
                Simulator::Schedule (Seconds (m_linkStatusCheckDelay), &BpLtpClaProtocol::CheckLinkStatus, this);
                return;
            }
        }
        it++;
    }
}

void
BpLtpClaProtocol::SendBundleFromTxQueue (uint64_t dstLtpEngineId)
{
    NS_LOG_FUNCTION (this);

    std::map<uint64_t, std::deque<TxQueueVals> >::iterator it = m_txQueueMap.find (dstLtpEngineId);
    if (it != m_txQueueMap.end ())
    {
        if (it->second.size () > 0)
        {
            TxQueueVals txQueueVals = it->second.front ();
            //it->second.pop (); // -- don't pop this off until after the Tx is complete; occurs in NotificationCallback
            uint64_t ClientServiceId = 1; // 1 - bundle protocol
            NS_LOG_FUNCTION (this << " Internal engine id: " << m_LtpEngineId << " Sending bundle from txQueue to " << txQueueVals.dstLtpEngineId << " with redSize of " << txQueueVals.redSize << " immediately");
            m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, txQueueVals.dstLtpEngineId, txQueueVals.cborBundle, txQueueVals.redSize);
        }
    }
    else
    {
        NS_LOG_FUNCTION (this << " Unable to find txQueue for dstLtpEngineId: " << dstLtpEngineId);
    }
    
}

int
BpLtpClaProtocol::SendPacket (Ptr<Packet> packet)
{
    NS_LOG_FUNCTION (this);
    // No operation for Ltp

    return -1;
}

int
BpLtpClaProtocol::EnableReceive (const BpEndpointId &local)
{
    NS_LOG_FUNCTION (this << local.Uri ());

    SetL4Callbacks ();
    return 0;
}

int
BpLtpClaProtocol::DisableReceive (const BpEndpointId &local)
{
    NS_LOG_FUNCTION (this << local.Uri ());
    
    m_ltp->UnregisterClientService (1); // 1 - bundle protocol
    return 0;
}

int
BpLtpClaProtocol::EnableSend (const BpEndpointId &src, const BpEndpointId &dst)
{
    NS_LOG_FUNCTION (this << " " << src.Uri () << " " << dst.Uri ());
    
    // No operation for Ltp
    // But, can maybe verify that target eid has route in BP route, the next hop L4address is registered, etc...
    // Perhaps have a mapping from remote eid to status to indicate whether it's ok to send or not...
    return 0;
}
/*
void
BpLtpClaProtocol::DisableSend (const BpEndpointId &remote)
{
    NS_LOG_FUNCTION (this << remote);
    // No operation for Ltp
}
*/
void
BpLtpClaProtocol::SetL4Callbacks ()
{
    NS_LOG_FUNCTION (this);
    // Ltp appears to only have a single callback function - Notification
    SetNotificationCallback();
}

int
BpLtpClaProtocol::SetL4Address (BpEndpointId eid, uint64_t l4Address)
{
    NS_LOG_FUNCTION (this << " " << eid.Uri () << " " << l4Address);

    std::map<BpEndpointId, uint64_t>::iterator it = m_endpointIdToLtpEngineId.find (eid);
    if (it == m_endpointIdToLtpEngineId.end ())
    {
        m_endpointIdToLtpEngineId.insert (std::pair<BpEndpointId, uint64_t>(eid, l4Address));
    }
    else
    {
        return -1;
    }
    return 0;
}

uint64_t
BpLtpClaProtocol::GetL4Address (BpEndpointId eid, uint64_t returnType)
{
    NS_LOG_FUNCTION (this << " " << eid.Uri ());
    
    std::map<BpEndpointId, uint64_t>::iterator it = m_endpointIdToLtpEngineId.find (eid);
    if (it != m_endpointIdToLtpEngineId.end ())
    {
        return it->second;
    }
    return -1;
}

InetSocketAddress
BpLtpClaProtocol::GetL4Address (BpEndpointId eid, InetSocketAddress returnType)
{
    NS_LOG_FUNCTION (this << " " << eid.Uri () << " Not the correct method for Ltp!");
    return InetSocketAddress ("127.0.0.1", 0);
}

int
BpLtpClaProtocol::SetL4Address (BpEndpointId eid, const InetSocketAddress* l4Address)
{
    //uint64_t ltpAddress = static_cast<uint64_t> (l4Address);
    const uint64_t* reinterpAddressPtr = reinterpret_cast<const uint64_t*> (l4Address);
    uint64_t ltpAddress = *reinterpAddressPtr;
    //std::memcpy (&ltpAddress, &l4Address, sizeof (ltpAddress));
    NS_LOG_FUNCTION (this << " " << eid.Uri () << " Extracted ltp address of " << ltpAddress);
    
    std::map<BpEndpointId, uint64_t>::iterator it = m_endpointIdToLtpEngineId.find (eid);
    if (it == m_endpointIdToLtpEngineId.end ())
    {
        m_endpointIdToLtpEngineId.insert (std::pair<BpEndpointId, uint64_t>(eid, ltpAddress));
    }
    else
    {
        return -1;
    }
    return 0;
}

/*
int
BpTcpClaProtocol::setL4AddressGeneric (BpEndpointId eid, unsigned char l4Address)
{
    NS_LOG_FUNCTION (this << eid << l4Address);
    // Implement this

    m_LtpEngineId = reinterpret_cast<uint64_t> (l4Address);
}

unsigned char
BpTcpClaProtocol::getL4AddressGeneric (BpEndpointId eid)
{
    NS_LOG_FUNCTION (this << eid);
    
    return reinterpret_cast<unsigned char> (m_LtpEngineId);
}
*/
void
BpLtpClaProtocol::SetRoutingProtocol (Ptr<BpRoutingProtocol> route)
{
    NS_LOG_FUNCTION (this << route);
    m_bpRouting = route;
}

Ptr<BpRoutingProtocol>
BpLtpClaProtocol::GetRoutingProtocol (void)
{
    NS_LOG_FUNCTION (this);
    return m_bpRouting;
}

//void
//BpLtpClaProtocol::SetL4Protocol (ns3::Ptr<ns3::ltp::LtpProtocol> protocol, uint64_t LtpEngineId)
void
BpLtpClaProtocol::SetL4Protocol (std::string l4Type)
{
    NS_LOG_FUNCTION (this << " " << l4Type);
    
    ns3::Ptr<ns3::ltp::LtpProtocol> protocol = m_bp->GetNode()->GetObject<ns3::ltp::LtpProtocol>();
    uint64_t LtpEngineId = protocol->GetLocalEngineId ();

    m_ltp = protocol;
    m_LtpEngineId = LtpEngineId;

    // Set callback functions of the transport layer
    SetLinkStatusChangeCallback ();
}
/*
void
BpLtpClaProtocol::SetL4Protocol (Ptr<Socket> protocol, InetSocketAddress l4address)
{
    NS_LOG_FUNCTION (this);
}
*/
Ptr<Socket>
BpLtpClaProtocol::GetL4Socket (Ptr<Packet> packet)
{
    NS_LOG_FUNCTION (this << packet);
    // Unsure if this needs to be implemented
    return Ptr <Socket> ();
}

void
BpLtpClaProtocol::SetLinkStatusCheckDelay (uint32_t delay)
{
    NS_LOG_FUNCTION (this << delay);
    m_linkStatusCheckDelay = delay;
}

// private

void
BpLtpClaProtocol::SetNotificationCallback (void)
{
    NS_LOG_FUNCTION (this);

    uint64_t ClientServiceId = 1; // 1 - bundle protocol
    
    CallbackBase NotifyCb = MakeCallback (&BpLtpClaProtocol::NotificationCallback, this);
    NS_LOG_FUNCTION (this << " NotifyCb: " << NotifyCb.GetImpl());
    m_ltp->RegisterClientService (ClientServiceId, NotifyCb);
}
/*
void
BpLtpClaProtocol::SetSendCallback (void)
{
    NS_LOG_FUNCTION (this);
    // Unsure if this needs to be implemented
}
*/
void
BpLtpClaProtocol::NotificationCallback (ns3::ltp::SessionId id,
                                        ns3::ltp::StatusNotificationCode code,
                                        std::vector<uint8_t> data,
                                        uint32_t dataLength,
                                        bool endFlag,
                                        uint64_t srcLtpEngine,
                                        uint32_t offset)
{
    //NS_LOG_FUNCTION (this << " NotificationCallback called with arguments: " << id << code << dataLength << endFlag << srcLtpEngine << offset);
    NS_LOG_FUNCTION (this << " Internal Engine ID " << m_LtpEngineId << " received NotificationCallback with code " << code << " :");
    // Implement this
    // This will be called when data is received, a transmission status is updated, or when a session is closed
    // Need to check if the data is a bundle or a status update
    // LTP Status Codes: RFC-5326 Sections 7.1 - 7.7
    //   SESSION_START
    //   GP_SEGMENT_RCV
    //   RED_PART_RCV
    //   TX_COMPLETED
    //   TX_SESSION_CANCEL
    //   RX_SESSION_CANCEL
    //   SESSION_END
    if (code == ns3::ltp::SESSION_START) // Sender receives noficiation that session has started
    {
        NS_LOG_FUNCTION (this << " Sender notified LTP session started. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " End flag: " << endFlag );
        //NS_LOG_FUNCTION (this << " Sender received notification that session " << id.GetSessionNumber () << " has started.  Record session information for later reference.");
        // Queue to generate new session for tracking.  What info should I keep?
        // Will need to iterate through m_txSessionMap, looking for an entry without a SessionId with a matching srcLtpEngineId
        // If no entry is found, error
        std::map<Ptr<BpBundle>, TxMapVals>::iterator it;
        for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
        {
            if (it->second.set == false) // this notification is called immediately after StartTransmission, so the first (only) entry not set should be the one we're looking for
            {
                it->second.sessionId = id;
                it->second.status = code;
                it->second.set = true; // indicate this is an entry with a matching SessionId
                NS_LOG_FUNCTION (this << " Session " << id.GetSessionNumber () << " has started.  Session information recorded for later reference.");
                break;
            }
        }
        if (it == m_txSessionMap.end ())
        {
            NS_LOG_ERROR (this << " No entry found in m_txSessionMap for srcLtpEngineId " << srcLtpEngine << ". m_txSessionMap contains following entries:");
            for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
            {
                NS_LOG_ERROR ("  " << this << "  Engine Id:" << it->second.srcLtpEngineId);
            }
        }
    }
    if (code == ns3::ltp::GP_SEGMENT_RCV)
    {
        NS_LOG_FUNCTION (this << " Receiver informed a Green Segment has been received. Internal engine ID: " << m_LtpEngineId << " Session Id: " << id.GetSessionNumber () << "; Data offset: " << offset << "; Segment length: " << dataLength << "; End flag: " << endFlag);
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
        if (it == m_rcvSessionMap.end ())
        {
            NS_LOG_FUNCTION (this << " First Green Part entry for SessionId " << id.GetSessionNumber ());
            RcvMapVals temp;
            temp.srcLtpEngineId = srcLtpEngine;
            temp.status = code;
            for ( std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
            {
                temp.greenData.push_back(*i);
            }
            temp.rcvGreenDataLength = temp.greenData.size ();
            NS_LOG_FUNCTION (this << "temp.greenData.size() = " << temp.greenData.size() << "; declared size: " << dataLength);
            temp.rcvRedDataLength = 0;  // indicate no red data received yet
            m_rcvSessionMap.insert(std::pair<ns3::ltp::SessionId, RcvMapVals>(id, temp));
        }
        else // Have received previous green segments for this session
        {
            NS_LOG_FUNCTION (this << " Subsequent Green Part entry for SessionId " << id.GetSessionNumber () );
            it->second.status = code;
            for ( std::vector<uint8_t>::const_iterator i = data.begin (); i != data.end (); ++i)
            {
                it->second.greenData.push_back(*i);
            }
            it->second.rcvGreenDataLength = it->second.greenData.size ();
            NS_LOG_FUNCTION (this << "it->second.greenData.size() = " << it->second.greenData.size() << "; declared size: " << dataLength);
        }
    }
    if (code == ns3::ltp::RED_PART_RCV)
    {
        NS_LOG_FUNCTION (this << " Receiver informed a Red Part has been received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Received << " << dataLength << " bytes");// << " Source LTP Engine ID: " << srcLtpEngine << " Segment Length: " << dataLength << " End Flag: " << endFlag);
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
        if (it == m_rcvSessionMap.end ())
        {
            NS_LOG_FUNCTION (this << " First Red Part entry for SessionId " << id.GetSessionNumber ());
            RcvMapVals temp;
            temp.srcLtpEngineId = srcLtpEngine;
            temp.status = code;
            for ( std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
            {
                temp.redData.push_back(*i);
            }
            temp.rcvRedDataLength = temp.redData.size ();
            temp.rcvGreenDataLength = 0;  // indicate no green data received yet
            m_rcvSessionMap.insert(std::pair<ns3::ltp::SessionId, RcvMapVals>(id, temp));
        }
        else // Have received previous red parts for this session
        {
            NS_LOG_FUNCTION (this << " Red Part entry for existing SessionId " << id.GetSessionNumber () );
            it->second.status = code;
            for ( std::vector<uint8_t>::const_iterator i = data.begin (); i != data.end (); ++i)
            {
                it->second.redData.push_back(*i);
            }
            it->second.rcvRedDataLength = it->second.redData.size ();
            NS_LOG_FUNCTION (this << "it->second.redData.size() = " << it->second.redData.size() << "; declared size: " << dataLength);
        }
    }
    if (code == ns3::ltp::SESSION_END)
    {
        NS_LOG_FUNCTION (this << " Informed Session Ended. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine);

        std::map<Ptr<BpBundle>, TxMapVals>::iterator TxIt;
        for (TxIt = m_txSessionMap.begin (); TxIt != m_txSessionMap.end (); TxIt++)
        {
            if (TxIt->second.sessionId == id)
            {
                TxIt->second.status = code;  // End of a transmission session.  Update status and return -- WHEN SHOULD THIS RECORD BE REMOVED? 
                NS_LOG_FUNCTION(this << " Sender notified session ID: " << id.GetSessionNumber () << " status updated.  Transmission session closed.");
                // Once Sender is informed existing Tx is complete, pop the tx record off m_txQueue and check if more are ready to send
                std::map<uint64_t, std::deque<TxQueueVals> >::iterator txQueueMapIt = m_txQueueMap.find (TxIt->second.dstLtpEngineId);
                if (txQueueMapIt == m_txQueueMap.end ())
                {
                    NS_LOG_FUNCTION (this << " Unable to find txQueue for dstLtpEngineId: " << TxIt->second.dstLtpEngineId);
                    return;
                }
                txQueueMapIt->second.pop_front (); // -- pop the bundle off the queue now that the Tx is complete
                uint32_t txQueueSize  = txQueueMapIt->second.size ();
                if (txQueueSize > 0)
                {
                    NS_LOG_FUNCTION (this << txQueueSize << " bundles remaining to send, scheduling to send next bundle immediately");
                    Simulator::ScheduleNow(&BpLtpClaProtocol::SendBundleFromTxQueue, this, TxIt->second.dstLtpEngineId);
                }
                else // Queue is empty, so delete it from the map
                {
                    NS_LOG_FUNCTION (this << " No bundles remaining to send, deleting queue");
                    m_txQueueMap.erase (TxIt->second.dstLtpEngineId);
                }
                return;
            }
        }
        
        // if not found in TxSessionMap, then this is the end of a reception

        NS_LOG_FUNCTION (this << "Receiver with engineID " << m_LtpEngineId << " informed SessionId " << id.GetSessionNumber () << " is complete");
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator RcvIt = m_rcvSessionMap.find (id);
        if (RcvIt == m_rcvSessionMap.end ())
        {
            NS_LOG_DEBUG (this << "Receiver with engineID " << m_LtpEngineId << " does not have " << id.GetSessionNumber () << " in m_rxSessionMap");
        }
        else
        {
            NS_LOG_FUNCTION (this << " SessionId " << id.GetSessionNumber () << " found in m_rxSessionMap");
            if (RcvIt->second.rcvRedDataLength == 0 && RcvIt->second.rcvGreenDataLength == 0)
            {
                NS_LOG_DEBUG (this << "No Data Received for SessionId " << id.GetSessionNumber () << ". Erasing session from m_rxSessionMap and returning");
                m_rcvSessionMap.erase(RcvIt);
            }
            else
            {
                // Assemble the bundle and send up to bundle protocol
                std::vector <uint8_t> assembledData (RcvIt->second.redData);
                /*
                std::vector <uint8_t> assembledData;
                if (RcvIt->second.rcvRedDataLength > 0)
                {
                    NS_LOG_FUNCTION (this << "Red Data Received for SessionId " << id.GetSessionNumber () << " with length " << RcvIt->second.redData.size() << " bytes (declared size of " << RcvIt->second.rcvRedDataLength << " bytes)");
                    // Assemble the bundle and send up to bundle protocol
                    for ( std::vector<uint8_t>::const_iterator i = RcvIt->second.redData.begin(); i != RcvIt->second.redData.end(); ++i)
                    {
                        assembledData.push_back(*i);
                    }
                }
                */
                if (RcvIt->second.rcvGreenDataLength > 0)
                {
                    NS_LOG_FUNCTION (this << "Green Data Received for SessionId " << id.GetSessionNumber () << " with length " << RcvIt->second.greenData.size() << " bytes (declared size of " << RcvIt->second.rcvGreenDataLength << " bytes)");
                    // Assemble the bundle and send up to bundle protocol
                    //std::vector <uint8_t> assembledData;
                    for ( std::vector<uint8_t>::const_iterator i = RcvIt->second.greenData.begin(); i != RcvIt->second.greenData.end(); ++i)
                    {
                        assembledData.push_back(*i);
                    }
                }
                if (assembledData.size() == 0)
                {
                    NS_LOG_DEBUG (this << " No Data Received for SessionId " << id.GetSessionNumber () << ". Erasing session from m_rxSessionMap and returning");
                    m_rcvSessionMap.erase(RcvIt);
                }
                else
                {
                    NS_LOG_FUNCTION (this << " Receiver with engineID " << m_LtpEngineId << " passing bundle with CBOR size of " << assembledData.size() << " bytes to bundle protocol");
                    //m_bp->ReceiveCborVector (assembledData);
                    Ptr<BpBundle> assembledBundle = AssembleBundle (assembledData, RcvIt->second.rcvRedDataLength);
                    m_bp->ReceiveBundle (assembledBundle);
                    m_rcvSessionMap.erase(RcvIt);
                }
            }
        }
    }
    if (code == ns3::ltp::TX_COMPLETED)
    {
        NS_LOG_FUNCTION (this << " Initial Session Transmission Completed.  Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine);
        
        if (srcLtpEngine == m_LtpEngineId) // Indicates the end of a transmission
        { 
            std::map<Ptr<BpBundle>, TxMapVals>::iterator it;
            for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
            {
                if (it->second.set && it->second.sessionId == id)
                {
                    it->second.status = code;  // NOTE: This only means initial Tx is complete - re-TX at LTP level may stil occur as needed
                    break;
                }
            }
        }
        else // Indicates the end of a receipt
        {
            NS_LOG_FUNCTION (this << " End of receipt of transmission for SessionId " << id.GetSessionNumber () << " received by receiver with engineID " << m_LtpEngineId);
        }

    }
    
}

std::vector<uint8_t>
BpLtpClaProtocol::SplitBundle (Ptr<BpBundle> bundle, RedDataModes redDataMode, uint64_t *redSize)
{
    NS_LOG_FUNCTION (this << " redDataMode: " << redDataMode);
    uint32_t cborSize = bundle->GetCborEncodingSize ();
    NS_LOG_FUNCTION (this << " Original bundle CBOR size is " << cborSize << " bytes");

    Ptr<BpBundle> splitBundle = CreateObject<BpBundle> ();
    splitBundle->SetRetentionConstraint(bundle->GetRetentionConstraint ());
    Ptr<BpBundle> greenBundle = CreateObject<BpBundle> ();
    greenBundle->SetRetentionConstraint(bundle->GetRetentionConstraint ());
    if (redDataMode == RED_DATA_SLIM)
    {
        // iterate through json fields looking for block with block number 0 (primary block)
        //splitBundle->SetPrimaryBlock(&(bundle->GetPrimaryBlock ()));
        if (splitBundle->AddBlocksFromBundle(bundle, BpCanonicalBlock::BLOCK_TYPE_PRIMARY, false) < 0) // 0 is block number for primary block
        {
            NS_LOG_FUNCTION (this << " Unable to get primary block from original bundle. Unable to send");
            return std::vector<uint8_t> ();
        }
        if (greenBundle->AddBlocksFromBundleExcept(bundle, BpCanonicalBlock::BLOCK_TYPE_PRIMARY, false) < 0) // 0 is block number for primary block
        {
            NS_LOG_FUNCTION (this << " Unable to get non-primary blocks from original bundle. Unable to send");
            return std::vector<uint8_t> ();
        }
    }
    else // RED_DATA_ROBUST
    {
        if (splitBundle->AddBlocksFromBundleExcept(bundle, BpCanonicalBlock::BLOCK_TYPE_PAYLOAD, false) < 0) // 1 is block number for payload block
        {
            NS_LOG_FUNCTION (this << " Unable to get all blocks except payload from original bundle. Unable to send");
            return std::vector<uint8_t> ();
        }
        if (greenBundle->AddBlocksFromBundle(bundle, BpCanonicalBlock::BLOCK_TYPE_PAYLOAD, false) < 0) // 1 is block number for payload block
        {
            NS_LOG_FUNCTION (this << " Unable to get payload block from original bundle. Unable to send");
            return std::vector<uint8_t> ();
        }
    }

    // Split bundles created, now generate CBOR vectors and combine
    std::vector<uint8_t> splitVector = splitBundle->GetCborEncoding ();
    std::vector<uint8_t> greenVector = greenBundle->GetCborEncoding ();
    *redSize = splitVector.size ();
    splitVector.insert (splitVector.end (), greenVector.begin (), greenVector.end ());
    NS_LOG_FUNCTION (this << " split CBOR vector has size: " << splitVector.size () << " with red part size of: " << *redSize);
    return splitVector;
}


Ptr<BpBundle>
BpLtpClaProtocol::AssembleBundle (std::vector<uint8_t> data, uint64_t redSize)
{
    NS_LOG_FUNCTION (this << " data.size() = " << data.size() << "; redSize = " << redSize);
    Ptr<BpBundle> bundle = CreateObject<BpBundle> ();
    if (redSize > 0 && redSize < data.size ())
    {
        // redSize is the amount of the vector, in bytes, that is red data
        // Since the vector is of type uint8_t, each element is 1 byte, making redSize usable as an index for the vector
        std::vector<uint8_t> redData (data.begin (), data.begin () + redSize);
        std::vector<uint8_t> greenData (data.begin () + redSize, data.end ());
        //NS_ASSERT (redData.size () == redSize);
        if (redData.size () != redSize)
        {
            NS_LOG_FUNCTION (this << " redData.size() = " << redData.size () << " != redSize = " << redSize);
            return bundle;
        }
        Ptr<BpBundle> redBundle = CreateObject<BpBundle> ();
        if ( redBundle->SetBundleFromCbor (redData) < 0 )
        {
            NS_LOG_FUNCTION (this << " Error processing red bundle.  Dropping data and returning empty bundle");
            return CreateObject<BpBundle> ();
        }

        Ptr<BpBundle> greenBundle = CreateObject<BpBundle> ();
        if ( greenBundle->SetBundleFromCbor (greenData) < 0 )
        {
            NS_LOG_FUNCTION (this << " Error processing green bundle.  Dropping green portion and returning red bundle");
            return redBundle;
        }
        // Have both red and green portions - combine them into a single bundle

        if (redBundle->AddBlocksFromBundle(greenBundle) < 0)
        {
            NS_LOG_FUNCTION (this << " Error adding green portion to red bundle. Dropping green portion and returning red bundle");
            return redBundle;
        }


        //json::iterator it = greenBundle->begin();
        //for (; it != greenBundle->end(); ++it)
        //{
        //    if (redBundle->AddToBundle(it.key(), it.value()) < 0)
        //    {
        //        NS_LOG_FUNCTION (this << " Error adding green portion to red bundle.  Dropping green portion and returning red bundle");
        //        return redBundle;
        //    }
        //    //greenBundle[it.key()] = it.value();
        //}
        // red and green data combined into redBundle
        //bundle = redBundle;
        bundle->SetBundleFromJson (redBundle->GetJson ());
        NS_LOG_FUNCTION (this << " Bundle assembled from both red and green parts; bundle payload has size: " << bundle->GetPayloadBlockPtr ()->GetBlockDataSize () << " bytes");
    }
    else
    {
        int retVal = bundle->SetBundleFromCbor (data);
        if (retVal < 0)
        {
            NS_LOG_FUNCTION (this << " Error processing bundle.  Dropping data and returning empty bundle");
            return CreateObject<BpBundle> ();
        }
    }
    return bundle;
}

void
BpLtpClaProtocol::LinkStatusChangeCallback(void)//(bool linkIsUp)
{
    //NS_LOG_FUNCTION (this << linkIsUp);
    NS_LOG_FUNCTION (this);
    //if (linkIsUp)
    //{
    //    CheckForBundleToSendFromTxQueue ();
    //}
    CheckForBundleToSendFromTxQueue (LINK_WAITING_LINK_UP_CB);
}

void
BpLtpClaProtocol::SetLinkStatusChangeCallback (void)
{
    NS_LOG_FUNCTION (this);
    Ptr<Ipv4> ipv4 = m_bp->GetNode ()->GetObject<Ipv4> ();
    for (uint32_t i = 1; i < ipv4->GetNInterfaces (); i++) // 0 is loopback; add callbacks on all of the nodes interfaces
    {
        //Ptr<Ipv4Interface> interface = ipv4->GetInterface (i);
        Ptr<NetDevice> netDevice = ipv4->GetNetDevice (i);
        
        NS_LOG_FUNCTION (this << " Adding link status change callback for interface " << i);
        
        netDevice->AddLinkChangeCallback (MakeCallback (&BpLtpClaProtocol::LinkStatusChangeCallback, this));
    }
}

void
BpLtpClaProtocol::CheckForBundleToSendFromTxQueue(BpLtpClaProtocol::LinkWaitingStatus linkWaitStatus)
{
    NS_LOG_FUNCTION (this << linkWaitStatus);
    // Check the first bundle in each queue to see if it's waiting on available link
    std::map<uint64_t, std::deque<TxQueueVals> >::iterator it = m_txQueueMap.begin ();
    for (; it != m_txQueueMap.end (); ++it)
    {
        if (it->second.size () > 0)
        {
            TxQueueVals txQueueVals = it->second.front ();
            NS_LOG_FUNCTION (this << " Checking bundle in txQueue for dstLtpEngineId: " << txQueueVals.dstLtpEngineId << " with wait status of: " << txQueueVals.waitStatus);
            if (txQueueVals.waitStatus == linkWaitStatus)
            {
                txQueueVals.waitStatus = LINK_WAITING_STATUS_CHECK;
                it->second.pop_front ();
                it->second.push_front (txQueueVals);
                NS_LOG_FUNCTION (this << " Checking for available link to destination engine id: " << txQueueVals.dstLtpEngineId);
                SendIfLinkUp (txQueueVals.dstLtpEngineId);
            }
        }   
    }
}

// A clumsy way to verify link availability before passing bundle to Ltp since
// Ltp does not do so prior to sending packet -- results in loss of Green data
// whenever sending data prior to link being available
int
BpLtpClaProtocol::SendIfLinkUp(uint64_t dstLtpEngineId)
{
    NS_LOG_FUNCTION (this << dstLtpEngineId);
    bool ableToSend = false;

    InetSocketAddress destAddr = m_ltp->GetBindingFromLtpEngineId (dstLtpEngineId);
    if (destAddr == InetSocketAddress ("127.0.0.1", 0))
    {
        NS_LOG_FUNCTION (this << "Unable to find destination address for LtpEngineId: " << dstLtpEngineId);
        //return -1;
    }
    else
    {
        Ptr<Ipv4> nodeIpv4 = m_bp->GetNode ()->GetObject<Ipv4> ();
        Ptr<Ipv4RoutingProtocol> routingProtocol = nodeIpv4->GetRoutingProtocol ();
        Ipv4Header ipv4Header;
        ipv4Header.SetDestination(destAddr.GetIpv4 ());
        Socket::SocketErrno sockErr;
        Ptr<Ipv4Route> route = routingProtocol->RouteOutput(NULL, ipv4Header, 0, sockErr);
        if (sockErr != Socket::ERROR_NOTERROR || !route)
        {
            NS_LOG_FUNCTION (this << " No route to destination address: " << destAddr.GetIpv4 ());
            //return -1;
        }
        else
        {
            NS_LOG_FUNCTION (this << " Route exists to destination address: " << destAddr.GetIpv4 () << " checking interface and device status");
            uint32_t outputIfIndex = nodeIpv4->GetInterfaceForDevice (route->GetOutputDevice ());
            // Check if both Ipv4 Interface and NetDevice are up
            //Ptr<Ipv4Interface> interface = nodeIpv4->GetInterface (outputIfIndex);
            //if (!interface->IsUp ())
            Ptr<NetDevice> netDevice = nodeIpv4->GetNetDevice (outputIfIndex);
            if (!nodeIpv4->IsUp (outputIfIndex) || !netDevice->IsLinkUp ())
            {
                NS_LOG_FUNCTION (this << " No interface or device available for interface index " << outputIfIndex);
                //return -1;
            }
            else
            {
                ableToSend = true;
            }
        }
    }
    if (!ableToSend)
    {
        std::map<uint64_t, std::deque<TxQueueVals> >::iterator it = m_txQueueMap.find (dstLtpEngineId);
        if (it != m_txQueueMap.end ())
        {
            if (it->second.size () > 0)
            {
                TxQueueVals txQueueVals = it->second.front ();
                if (txQueueVals.waitStatus == LINK_WAITING_STATUS_CHECK)
                {
                    txQueueVals.waitStatus = LINK_WAITING_LINK_UP_CB; // change wait status to wait for link up callback
                    it->second.pop_front ();
                    it->second.push_front (txQueueVals);
                }
            }
        }
        return -1;
    }
    
    NS_LOG_FUNCTION (this << " Route and link available to LtpEngineId: " << dstLtpEngineId);
    SendBundleFromTxQueue (dstLtpEngineId);
    return 0;
}


/*
int
BpLtpClaProtocol::SendIfLinkUp(uint64_t dstLtpEngineId)
{
    NS_LOG_FUNCTION (this << dstLtpEngineId);

    InetSocketAddress destAddr = m_ltp->GetBindingFromLtpEngineId (dstLtpEngineId);
    if (destAddr == InetSocketAddress ("127.0.0.1", 0))
    {
        NS_LOG_FUNCTION (this << "Unable to find destination address for LtpEngineId: " << dstLtpEngineId);
        return -1;
    }

    TypeId tid;
    tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    Ptr<Socket> socket = Socket::CreateSocket (m_bp->GetNode (), tid);

    if (socket->Bind () != 0)
    {
        NS_LOG_FUNCTION (this << " Unable to bind socket");
        return -1;
    }
    socket->SetConnectCallback(MakeCallback (&BpLtpClaProtocol::ConnectionRequestSucceededCallback, this),
                               MakeCallback (&BpLtpClaProtocol::ConnectionRequestFailedCallback, this));

    m_socketToLtpEngineId.insert (std::pair<Ptr<Socket>, uint64_t>(socket, dstLtpEngineId));

    if (socket->Connect (destAddr) != 0)
    {
        NS_LOG_FUNCTION (this << " Unable to connect to destination address: " << destAddr);
        socket->Close();
        return -1;
    }
    NS_LOG_FUNCTION (this << " Connection request pending for dest LtpEngineID: " << dstLtpEngineId << " with address: " << destAddr.GetIpv4 () << "; waiting for callback");
    //socket->Close();  // Close this in the callback
    return 0;
}
*/

void
BpLtpClaProtocol::ConnectionRequestSucceededCallback (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);

    std::map<Ptr<Socket>, uint64_t>::iterator it = m_socketToLtpEngineId.find(socket);
    if (it == m_socketToLtpEngineId.end ())
    {
        NS_LOG_FUNCTION (this << "Unable to find socket in m_socketToLtpEngineId");
        return;
    }
    uint64_t dstLtpEngineId = it->second;
    m_socketToLtpEngineId.erase(socket);

    NS_LOG_FUNCTION (this << " Connection request succeeded to EngineID: " << dstLtpEngineId);
    // Now process Send Queue
    SendBundleFromTxQueue (dstLtpEngineId);

    socket->Close();
}

void
BpLtpClaProtocol::ConnectionRequestFailedCallback (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    socket->Close();
    // Connection request failed, so need to wait for linkstatuschange callback
    Address address;
    socket->GetSockName (address);
    InetSocketAddress destAddr = InetSocketAddress::ConvertFrom (address);
    uint64_t dstLtpEngineId = m_ltp->GetBindingFromIpv4Addr (destAddr);

    std::map<uint64_t, std::deque<TxQueueVals> >::iterator it = m_txQueueMap.find (dstLtpEngineId);
    if (it != m_txQueueMap.end ())
    {
        if (it->second.size () > 0)
        {
            TxQueueVals txQueueVals = it->second.front ();
            if (txQueueVals.waitStatus == LINK_WAITING_STATUS_CHECK)
            {
                txQueueVals.waitStatus = LINK_WAITING_LINK_UP_CB; // change wait status to wait for link up callback
                it->second.pop_front ();
                it->second.push_front (txQueueVals);
            }
        }
    }
}

int
BpLtpClaProtocol::CheckL4LinkToEid(BpEndpointId nextHopEid, Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << nextHopEid.Uri ());

    uint64_t returnType;
    uint64_t nextHopEngineId = GetL4Address (nextHopEid, returnType);
    if (nextHopEngineId == -1)
    {
        NS_LOG_FUNCTION (this << " Unable to get L4 address for eid: " << nextHopEid.Uri ());
        return -1;
    }
    InetSocketAddress nextHopAddr = m_ltp->GetBindingFromLtpEngineId (nextHopEngineId);
    if (nextHopAddr == InetSocketAddress ("127.0.0.1", 0))
    {
        NS_LOG_FUNCTION (this << "Unable to find destination address for LtpEngineId: " << nextHopEngineId);
        return -1;
    }
    Ptr<Ipv4> nodeIpv4 = m_bp->GetNode ()->GetObject<Ipv4> ();
    Ptr<Ipv4RoutingProtocol> routingProtocol = nodeIpv4->GetRoutingProtocol ();
    Ipv4Header ipv4Header;
    ipv4Header.SetDestination(nextHopAddr.GetIpv4 ());
    Socket::SocketErrno sockErr;
    Ptr<Ipv4Route> route = routingProtocol->RouteOutput(NULL, ipv4Header, 0, sockErr);
    if (sockErr != Socket::ERROR_NOTERROR || !route)
    {
        NS_LOG_FUNCTION (this << " No route to destination address: " << nextHopAddr.GetIpv4 ());
        return -1;
    }
    uint32_t outputIfIndex = nodeIpv4->GetInterfaceForDevice (route->GetOutputDevice ());
    Ptr<NetDevice> netDevice = nodeIpv4->GetNetDevice (outputIfIndex);
    if (!nodeIpv4->IsUp (outputIfIndex) || !netDevice->IsLinkUp ())
    {
        NS_LOG_FUNCTION (this << " No interface or device available for interface index " << outputIfIndex);
        return -1;
    }

    if (socket->Connect (nextHopAddr) != 0)
    {
        NS_LOG_FUNCTION (this << " Unable to initiate connection to destination address: " << nextHopAddr);
        socket->Close();
        return -1;
    }
    NS_LOG_FUNCTION (this << " Connection request pending for " << nextHopEid.Uri () << " with address: " << nextHopAddr.GetIpv4 () << "; waiting for callback");
    return 0;

}

void
BpLtpClaProtocol::L4LinkToEidConnectionRequestSucceededCallback (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);

    UpdateL4LinkToEidSocketStatus (socket, 1);
}

void
BpLtpClaProtocol::L4LinkToEidConnectionRequestFailedCallback (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);

    UpdateL4LinkToEidSocketStatus (socket, 2);
}

void
BpLtpClaProtocol::UpdateL4LinkToEidSocketStatus (Ptr<Socket> socket, uint8_t socketStatus)
{
    NS_LOG_FUNCTION (this << socket << socketStatus);

    // Finished performing operations on socket, so close it
    socket->Close ();

    std::vector<TxLinkCheckVectorVals>::iterator it = m_txLinkStatusVector.begin ();
    for (; it != m_txLinkStatusVector.end (); ++it)
    {
        if (it->primarySocket == socket || it->alternateSocket == socket)
        {
            break;
        }
    }
    if (it == m_txLinkStatusVector.end ())
    {
        NS_LOG_FUNCTION (this << "Unable to find socket in m_txLinkStatusVector");
        return;
    }
    if (it->primarySocket == socket)
    {
        it->primaryRouteStatus = socketStatus;
    }
    else
    {
        it->alternateRouteStatus = socketStatus;
    }
    if (it->primaryRouteStatus == 0 || it->alternateRouteStatus == 0)
    {
        NS_LOG_FUNCTION (this << "Waiting for other socket to respond");
        return;
    }
    TxLinkCheckVectorVals txLinkCheckVectorVals = *it;
    m_txLinkStatusVector.erase (it);

    BpEndpointId nextHop;
    if (txLinkCheckVectorVals.primaryRouteStatus == 1 && txLinkCheckVectorVals.alternateRouteStatus == 1)
    {
        NS_LOG_FUNCTION (this << "Both routes are up, using primary route");
        nextHop = txLinkCheckVectorVals.primaryDstEid;
    }
    else if (txLinkCheckVectorVals.primaryRouteStatus == 1 && txLinkCheckVectorVals.alternateRouteStatus == 2)
    {
        NS_LOG_FUNCTION (this << "Alternate route failed, using primary route");
        nextHop = txLinkCheckVectorVals.primaryDstEid;
    }
    else if (txLinkCheckVectorVals.primaryRouteStatus == 2 && txLinkCheckVectorVals.alternateRouteStatus == 1)
    {
        NS_LOG_FUNCTION (this << "Primary route failed, using alternate route");
        nextHop = txLinkCheckVectorVals.alternateDstEid;
    }
    else
    {
        NS_LOG_FUNCTION (this << "Both routes failed, using primary route to wait");
        nextHop = txLinkCheckVectorVals.primaryDstEid;
    }
    // Proceed to send bundle to next hop
    SendBundleToNextHop (txLinkCheckVectorVals.bundle, nextHop);
}

} // namespace ns3
