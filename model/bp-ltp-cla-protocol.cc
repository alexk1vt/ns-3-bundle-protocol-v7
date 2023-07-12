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
      m_bpRouting (0)
{
    NS_LOG_FUNCTION (this);
}

BpLtpClaProtocol::~BpLtpClaProtocol ()
{
    NS_LOG_FUNCTION (this);
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
        // Mapping may need to wait until LtpSession is created - TODO: FIX THIS!!
        // Look up destination Engine Id based on destEID
        // Encapsulate bundle into a packet
        uint32_t cborSize = bundle->GetCborEncodingSize ();
        //Ptr<Packet> packet = Create<Packet> (reinterpret_cast<uint8_t*>(bundle->GetCborEncoding ()[0]), cborSize);
        //BpBundleHeader bundleHeader;
        //bundleHeader.SetBundleSize (cborSize);
        //packet->AddHeader (bundleHeader);
        std::vector <std::uint8_t> cborEncoding = bundle->GetCborEncoding ();
        NS_LOG_FUNCTION (this << "cborEncoding.size() = " << cborEncoding.size() << " cborEncoding.capacity() = " << cborEncoding.capacity() << " cborSize = " << cborSize);
        uint64_t ClientServiceId = 1; // 1 - bundle protocol
        uint64_t returnType;
        uint64_t destEngineId = GetL4Address (dst, returnType);
        if (destEngineId == -1)
        {
            NS_LOG_FUNCTION (this << " Unable to get L4 address for eid: " << dst.Uri ());
            return -1;
        }
        uint64_t redSize = cborSize; // set entire bundle as red part for now
        NS_LOG_FUNCTION (this << " Sending packet sent from eid: " << src.Uri () << " to eid: " << dst.Uri () << " immediately");
        //m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, destEngineId, reinterpret_cast<const uint8_t*>(packet), redSize);
        m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, destEngineId, cborEncoding, 0);
        
        // store bundle pointer in Tx Session Map for later updating and tracking
        TxMapVals txMapVals;
        txMapVals.dstEid = dst;
        txMapVals.srcEid = src;
        txMapVals.srcLtpEngineId = m_LtpEngineId;
        txMapVals.dstLtpEngineId = destEngineId;
        txMapVals.set = false;
        std::map<Ptr<BpBundle>, TxMapVals>::iterator it = m_txSessionMap.find (bundle);
        if (it == m_txSessionMap.end ())
        {
            m_txSessionMap.insert (std::pair<Ptr<BpBundle>, TxMapVals>(bundle, txMapVals));
        }
        else
        {
            NS_LOG_FUNCTION (this << " Unable to insert bundle into txSessionMap");
            return -1;
        }
        return 0;
    }
    NS_LOG_FUNCTION (this << " Unable to get bundle for eid: " << src.Uri ());
    return -1;
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
    //m_ltp->RegisterClientService (
    //    ClientServiceId,
    //    MakeCallback (&BpLtpClaProtocol::NotificationCallback, this));
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
    NS_LOG_FUNCTION (this << "Internal Engine ID " << m_LtpEngineId << " received NotificationCallback:");
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
        NS_LOG_FUNCTION (this << " LTP Session Started. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine);
        //NS_LOG_FUNCTION (this << " Sender received notification that session " << id.GetSessionNumber () << " has started.  Record session information for later reference.");
        // Queue to generate new session for tracking.  What info should I keep?
        // Will need to iterate through m_txSessionMap, looking for an entry without a SessionId with a matching srcLtpEngineId
        // If no entry is found, error
        std::map<Ptr<BpBundle>, TxMapVals>::iterator it;
        for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
        {
            if (it->second.srcLtpEngineId == srcLtpEngine && it->second.set == false)
            {
                it->second.sessionId = id;
                it->second.status = code;
                it->second.set = true; // indicate this is an entry with a matching SessionId
                break;
            }
        }
        if (it == m_txSessionMap.end ())
        {
            NS_LOG_ERROR ("No entry found in m_txSessionMap for srcLtpEngineId " << srcLtpEngine);
        }
    }
    else if (code == ns3::ltp::GP_SEGMENT_RCV)
    {
        //NS_LOG_FUNCTION (this << " Green Segment Received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine << " Data offset: " << offset << " Segment Length: " << dataLength << " End Flag: " << endFlag);
        NS_LOG_FUNCTION (this << " Receiver is informed that a Green Segment has been received.  This may be the first transmission for a new session.  Session Id is " << id.GetSessionNumber () << ".  Data offset is " << offset << ".  Segment length is " << dataLength << ".  Is this the last transmission? End flag is " << endFlag);
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
        if (it == m_rcvSessionMap.end ())
        {
            NS_LOG_FUNCTION (this << "First Green Part entry for SessionId " << id.GetSessionNumber ());
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
            NS_LOG_FUNCTION (this << "Found previous Green Parts Received for SessionId " << id.GetSessionNumber () );
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
        //NS_LOG_FUNCTION (this << " Red Part Received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine << " Segment Length: " << dataLength << " End Flag: " << endFlag);
        NS_LOG_FUNCTION (this << " Red Part Received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Received << " << dataLength << " bytes");// << " Source LTP Engine ID: " << srcLtpEngine << " Segment Length: " << dataLength << " End Flag: " << endFlag);
        std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
        if (it == m_rcvSessionMap.end ())
        {
            NS_LOG_FUNCTION (this << "First Red Part entry for SessionId " << id.GetSessionNumber ());
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
            NS_LOG_FUNCTION (this << "Found previous Red Parts Received for SessionId " << id.GetSessionNumber () << "- Should not happen!");
        }
    }
    else if (code == ns3::ltp::SESSION_END)
    {
        NS_LOG_FUNCTION (this << " Session Ended. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id.GetSessionNumber () << " Source LTP Engine ID: " << srcLtpEngine);
        
        if (srcLtpEngine == m_LtpEngineId) // indicating the end of a transmission
        {
            std::map<Ptr<BpBundle>, TxMapVals>::iterator it;
            for (it = m_txSessionMap.begin (); it != m_txSessionMap.end (); it++)
            {
                if (it->second.set && it->second.sessionId == id)
                {
                    it->second.status = code;  // I believe this indicates the session is fully completed - when should this entry be cleared out?
                    break;
                }
            }
        }
        else // indicating the end of a reception
        {
            NS_LOG_FUNCTION (this << "Receiver with engineID " << m_LtpEngineId << " informed SessionId " << id.GetSessionNumber () << " is complete");
            std::map<ns3::ltp::SessionId, RcvMapVals>::iterator it = m_rcvSessionMap.find (id);
            if (it == m_rcvSessionMap.end ())
            {
                NS_LOG_FUNCTION (this << "Receiver with engineID " << m_LtpEngineId << " does not have " << id.GetSessionNumber () << " in m_rxSessionMap");
            }
            else
            {
                NS_LOG_FUNCTION (this << "SessionId " << id.GetSessionNumber () << " found in m_rxSessionMap");
                if (it->second.rcvRedDataLength == 0 && it->second.rcvGreenDataLength == 0)
                {
                    NS_LOG_DEBUG (this << "No Data Received for SessionId " << id.GetSessionNumber ());
                    return;
                }
                // Assemble the bundle and send up to bundle protocol
                std::vector <uint8_t> assembledData;
                if (it->second.rcvRedDataLength > 0)
                {
                    NS_LOG_FUNCTION (this << "Red Data Received for SessionId " << id.GetSessionNumber () << " with length " << it->second.redData.size() << " bytes (declared size of " << it->second.rcvRedDataLength << " bytes)");
                    // Assemble the bundle and send up to bundle protoco
                    assembledData.resize (it->second.rcvRedDataLength);
                    std::copy(it->second.redData.begin(), it->second.redData.end(), assembledData.begin());
                }
                if (it->second.rcvGreenDataLength > 0)
                {
                    NS_LOG_FUNCTION (this << "Green Data Received for SessionId " << id.GetSessionNumber () << " with length " << it->second.greenData.size() << " bytes (declared size of " << it->second.rcvGreenDataLength << " bytes)");
                    // Assemble the bundle and send up to bundle protocol
                    //std::vector <uint8_t> assembledData;
                    assembledData.resize (assembledData.size() + it->second.rcvGreenDataLength);
                    std::copy(it->second.greenData.begin(), it->second.greenData.end(), assembledData.begin() + it->second.rcvRedDataLength);
                }
                if (assembledData.size() == 0)
                {
                    NS_LOG_DEBUG (this << " No Data Received for SessionId " << id.GetSessionNumber () << ". Erasing session from m_rxSessionMap and returning");
                    m_rcvSessionMap.erase(it);
                    return;
                }
                NS_LOG_FUNCTION (this << " Receiver with engineID " << m_LtpEngineId << " sending CBOR bundle with" << assembledData.size() << " bytes to bundle protocol");
                m_bp->ReceiveCborVector (assembledData);
                m_rcvSessionMap.erase(it);
            }
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