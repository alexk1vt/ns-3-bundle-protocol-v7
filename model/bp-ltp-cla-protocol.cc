/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bp-ltp-cla-protocol.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/object-vector.h"
#include "ns3/packet.h"

#include "ns3/node.h"
#include "ns3/simulator.h"

#include "ns3/config.h" // added by AlexK. to allow modifying default TCP values
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
      m_txCnt (0)
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

int
BpLtpClaProtocol::SendBundle (Ptr<BpBundle> bundlePtr)
{
    NS_LOG_FUNCTION (this << " " << bundlePtr);
    
    BpPrimaryBlock *bpPrimaryBlockPtr = bundlePtr->GetPrimaryBlockPtr ();
    BpEndpointId src = bpPrimaryBlockPtr->GetSourceEid ();
    BpEndpointId dst = bpPrimaryBlockPtr->GetDestinationEid ();
    Ptr<BpBundle> bundle = m_bp->GetBundle (src); // this is retrieved again here in order to pop the packet from the SendBundleStore! -- TODO: FIX THIS!!

    if (bundle)
    {
        // Send the bundle
        // Create mapping to keep track of bundle and its ongoing corresponding Ltp Session
        // Mapping may need to wait until LtpSession is created
        // Look up destination Engine Id based on destEID

        BpEndpointId internalEid = m_bp->GetBpEndpointId ();
        
        uint64_t ClientServiceId = 1; // 1 - bundle protocol
        Ptr<BpStaticRoutingProtocol> route = DynamicCast<BpStaticRoutingProtocol> (m_bpRouting);
        BpEndpointId nextHopEid = route->GetRoute (dst);
        uint64_t returnType;
        uint64_t nextHopEngineId = GetL4Address (nextHopEid, returnType);
        if (nextHopEngineId == -1)
        {
            NS_LOG_FUNCTION (this << " Unable to get L4 address for eid: " << nextHopEid.Uri ());
            return -1;
        }
        
        //uint64_t redSize = bundle->GetCborEncodingSize ();  // set entire bundle as red part for now
        uint64_t redSize = 0;
        
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
            return -1;
        }

        //NS_LOG_FUNCTION (this << " Internal engine id: " << m_LtpEngineId << " Sending packet sent from eid: " << src.Uri () << " to nextHopEid: " << nextHopEid.Uri () << " with nextHopEngineId: " << nextHopEngineId << " immediately");
        //m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, nextHopEngineId, cborEncoding, redSize);
        float_t txDelay = GetTxCnt() * (m_ltp->GetOneWayLightTime().GetSeconds() * 5);
        NS_LOG_FUNCTION (this << " Internal engine id: " << m_LtpEngineId << " Sending bundle from eid: " << src.Uri () << " to nextHopEid: " << nextHopEid.Uri () << " with nextHopEngineId: " << nextHopEngineId << " in " << txDelay << " seconds");
        Simulator::Schedule (Seconds (txDelay), &BpLtpClaProtocol::StartTransmission, this, bundle, nextHopEid, nextHopEngineId, redSize);
        IncrementTxCnt();
        return 0;
    }
    NS_LOG_FUNCTION (this << " Unable to get bundle for eid: " << src.Uri ());
    return -1;
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
    uint32_t cborSize = bundle->GetCborEncodingSize ();
    std::vector <uint8_t> cborEncoding = bundle->GetCborEncoding ();
    //std::vector <uint8_t> cborEncoding (cborSize, 65); // Red segment works for a vector of just '65s' but not for a vector of actual data...
    
    uint64_t ClientServiceId = 1; // 1 - bundle protocol

    DecrementTxCnt();
    NS_LOG_FUNCTION (this << " Internal engine id: " << m_LtpEngineId << " Sending packet sent from eid: " << internalEid.Uri () << " to nextHopEid: " << nextHopEid.Uri () << " with nextHopEngineId: " << nextHopEngineId << " of size " << cborEncoding.size() << " bytes immediately");
    m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, nextHopEngineId, cborEncoding, redSize);
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
    // But, can verify that target eid has route in BP route, the next hop L4address is registered, etc...
    // Perhaps have a mapping from remote eid to status to indicate whether it's ok to send or not...
    return -1;
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
    NS_LOG_FUNCTION (this << " " << eid. Uri () << " Not the correct method for Ltp!");
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
    NS_LOG_FUNCTION (this << " Internal Engine ID " << m_LtpEngineId << " received NotificationCallback:");
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
        NS_LOG_FUNCTION (this << " Sender notified LTP session started. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber ());
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
                return;
            }
        }
        if (it == m_txSessionMap.end ())
        {
            NS_LOG_ERROR (this << " No entry found in m_txSessionMap for srcLtpEngineId " << srcLtpEngine << ". m_txSessionMap contains following entries:");
            for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
            {
                NS_LOG_ERROR ("  " << this << "  Engine Id:" << it->second.srcLtpEngineId);
            }
            return;
        }
    }
    else if (code == ns3::ltp::GP_SEGMENT_RCV)
    {
        NS_LOG_FUNCTION (this << " Receiver informed a Green Segment has been received. Internal engine ID: " << m_LtpEngineId << " Session Id: " << id.GetSessionNumber () << "; Data offset: " << offset << "; Segment length: " << dataLength << "; End flag: " << endFlag);
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
        if (it == m_rcvSessionMap.end ())
        {
            NS_LOG_FUNCTION (this << " First Green Part entry for SessionId " << id.GetSessionNumber ());
            RcvMapVals temp;
            temp.srcLtpEngineId = srcLtpEngine;
            temp.status = code;
            temp.greenData.resize(dataLength + offset);
            std::copy(data.begin(), data.end(), temp.greenData.begin() + offset); // I'm assuming first green data received could have an offset
            temp.rcvGreenDataLength = dataLength;
            NS_LOG_FUNCTION (this << "temp.greenData.size() = " << temp.greenData.size() << " / temp.rcvGreenDataLength = " << temp.rcvGreenDataLength);
            temp.rcvRedDataLength = 0;  // indicate no red data received yet
            m_rcvSessionMap.insert(std::pair<ns3::ltp::SessionId, RcvMapVals>(id, temp));
        }
        else // Have received previous green segments for this session
        {
            NS_LOG_FUNCTION (this << " Subsequent Green Part entry for SessionId " << id.GetSessionNumber () );
            it->second.status = code;
            if (it->second.greenData.size() < it->second.rcvGreenDataLength + dataLength)
            {
                it->second.greenData.resize(it->second.greenData.size() + dataLength);
            }
            std::copy(data.begin(), data.end(), it->second.greenData.begin() + offset); // I'm assuming first green data received could have an offset
            it->second.rcvGreenDataLength += dataLength;
            it->second.status = code;
        }
    }
    else if (code == ns3::ltp::RED_PART_RCV)
    {
        NS_LOG_FUNCTION (this << " Receiver informed a Red Part has been received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Received << " << dataLength << " bytes");// << " Source LTP Engine ID: " << srcLtpEngine << " Segment Length: " << dataLength << " End Flag: " << endFlag);
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
        if (it == m_rcvSessionMap.end ())
        {
            NS_LOG_FUNCTION (this << " First Red Part entry for SessionId " << id.GetSessionNumber ());
            RcvMapVals temp;
            temp.srcLtpEngineId = srcLtpEngine;
            temp.status = code;
            temp.redData.resize(dataLength);
            std::copy(data.begin(), data.end(), temp.redData.begin());
            temp.rcvRedDataLength = dataLength;
            temp.rcvGreenDataLength = 0;  // indicate no green data received yet
            m_rcvSessionMap.insert(std::pair<ns3::ltp::SessionId, RcvMapVals>(id, temp));
        }
        else // Have received previous red parts for this session
        {
            NS_LOG_FUNCTION (this << " Subsequent Red Part entry for SessionId " << id.GetSessionNumber () << "- Should not happen!");
        }
    }
    else if (code == ns3::ltp::SESSION_END)
    {
        NS_LOG_FUNCTION (this << " Informed Session Ended. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine);
        

        std::map<Ptr<BpBundle>, TxMapVals>::iterator TxIt;
        for (TxIt = m_txSessionMap.begin (); TxIt != m_txSessionMap.end (); TxIt++)
        {
            if (TxIt->second.sessionId == id)
            {
                TxIt->second.status = code;  // End of a transmission session.  Update status and return -- WHEN SHOULD THIS RECORD BE REMOVED? 
                NS_LOG_FUNCTION(this << " Session ID: " << id.GetSessionNumber () << " status updated.  Transmission session closed.");
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
            NS_LOG_FUNCTION (this << "SessionId " << id.GetSessionNumber () << " found in m_rxSessionMap");
            if (RcvIt->second.rcvRedDataLength == 0 && RcvIt->second.rcvGreenDataLength == 0)
            {
                NS_LOG_DEBUG (this << "No Data Received for SessionId " << id.GetSessionNumber () << ". Erasing session from m_rxSessionMap and returning");
                m_rcvSessionMap.erase(RcvIt);
                return;
            }
            // Assemble the bundle and send up to bundle protocol
            std::vector <uint8_t> assembledData;
            if (RcvIt->second.rcvRedDataLength > 0)
            {
                NS_LOG_FUNCTION (this << "Red Data Received for SessionId " << id.GetSessionNumber () << " with length " << RcvIt->second.redData.size() << " bytes (declared size of " << RcvIt->second.rcvRedDataLength << " bytes)");
                // Assemble the bundle and send up to bundle protoco
                assembledData.resize (RcvIt->second.rcvRedDataLength);
                std::copy(RcvIt->second.redData.begin(), RcvIt->second.redData.end(), assembledData.begin());
            }
            if (RcvIt->second.rcvGreenDataLength > 0)
            {
                NS_LOG_FUNCTION (this << "Green Data Received for SessionId " << id.GetSessionNumber () << " with length " << RcvIt->second.greenData.size() << " bytes (declared size of " << RcvIt->second.rcvGreenDataLength << " bytes)");
                // Assemble the bundle and send up to bundle protocol
                //std::vector <uint8_t> assembledData;
                assembledData.resize (assembledData.size() + RcvIt->second.rcvGreenDataLength);
                std::copy(RcvIt->second.greenData.begin(), RcvIt->second.greenData.end(), assembledData.begin() + RcvIt->second.rcvRedDataLength);
            }
            if (assembledData.size() == 0)
            {
                NS_LOG_DEBUG (this << " No Data Received for SessionId " << id.GetSessionNumber () << ". Erasing session from m_rxSessionMap and returning");
                m_rcvSessionMap.erase(RcvIt);
                return;
            }
            NS_LOG_FUNCTION (this << " Receiver with engineID " << m_LtpEngineId << " passing CBOR bundle with" << assembledData.size() << " bytes to bundle protocol");
            m_bp->ReceiveCborVector (assembledData);
            m_rcvSessionMap.erase(RcvIt);
        }
        
    }
    else if (code == ns3::ltp::TX_COMPLETED)
    {
        NS_LOG_FUNCTION (this << " Initial Session Transmission Completed.  Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine);
        
        if (srcLtpEngine == m_LtpEngineId) // Indicates the end of a transmission
        { 
            std::map<Ptr<BpBundle>, TxMapVals>::iterator it;
            for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
            {
                if (it->second.set && it->second.sessionId == id)
                {
                    it->second.status = code;  // NOTE: This only means initial Tx is complete - re-TX may stil occur as needed
                    break;
                }
            }
        }
        else // Indicates the end of a receipt
        {
            return;
        }

    }
    
}


} // namespace ns3