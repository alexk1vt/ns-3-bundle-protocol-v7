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
        m_ltp -> StartTransmission (ClientServiceId, ClientServiceId, destEngineId, cborEncoding, redSize);
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
BpLtpClaProtocol::NotificationCallback (ns3::ltp::SessionId id, ns3::ltp::StatusNotificationCode code, std::vector<uint8_t> data, uint32_t dataLength, bool endFlag, uint64_t srcLtpEngine, uint32_t offset)
{
    NS_LOG_FUNCTION (this << id << code << data << dataLength << endFlag << srcLtpEngine << offset);
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
    if (code == ns3::ltp::SESSION_START) // Trying to see what information is recieved to identify whether this is a new session from a transmissiion start or an incoming session
    {
        NS_LOG_INFO (this << " LTP Session Started. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id << " Source LTP Engine ID: " << srcLtpEngine);
    }
    else if (code == ns3::ltp::GP_SEGMENT_RCV)
    {
        NS_LOG_INFO (this << " Green Segment Received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id << " Source LTP Engine ID: " << srcLtpEngine << " Data offset: " << offset << " Segment Length: " << dataLength << " End Flag: " << endFlag);
    }
    else if (code == ns3::ltp::RED_PART_RCV)
    {
        NS_LOG_INFO (this << " Red Part Received. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id << " Source LTP Engine ID: " << srcLtpEngine << " Segment Length: " << dataLength << " End Flag: " << endFlag);
    }
    else if (code == ns3::ltp::SESSION_END)
    {
        NS_LOG_INFO (this << " Session Ended. Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id << " Source LTP Engine ID: " << srcLtpEngine);
    }
    else if (code == ns3::ltp::TX_COMPLETED)
    {
        NS_LOG_INFO (this << " Initial Session Transmission Completed.  Internal Engine ID: " << m_LtpEngineId << " Session ID: " << id << " Source LTP Engine ID: " << srcLtpEngine);
    }
    
}


} // namespace ns3