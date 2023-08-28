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
 * 
 * * BP-7 Author: Alexander Kedrowitsch <alexk1@vt.edu>
 * 
 */

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/buffer.h"
#include "bp-tcp-cla-protocol.h"
#include "bp-ltp-cla-protocol.h"
#include "bundle-protocol.h"
//#include "bp-header.h"
//#include "bp-payload-header.h"
#include "bp-bundle.h"
#include "bp-primary-block.h"
#include "bp-canonical-block.h"
#include <algorithm>
#include <map>
#include <ctime>

#define RFC_DATE_2000 946684800

NS_LOG_COMPONENT_DEFINE ("BundleProtocol");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BundleProtocol);

TypeId
BundleProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BundleProtocol")
    .SetParent<Object> ()
    .AddConstructor<BundleProtocol> ()
    .AddAttribute ("BundleSize", "Max size of a bundle in bytes",
           UintegerValue (512),
           MakeUintegerAccessor (&BundleProtocol::m_bundleSize),
           MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("L4Type", "The type of transport layer protocol",
           StringValue ("Tcp"),
           MakeStringAccessor (&BundleProtocol::m_l4Type),
           MakeStringChecker ())
    .AddAttribute ("StartTime", "Time at which the bundle protocol will start",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&BundleProtocol::m_startTime),
                   MakeTimeChecker ())
    .AddAttribute ("StopTime", "Time at which the bundle protocol will stop",
                   TimeValue (TimeStep (0)),
                   MakeTimeAccessor (&BundleProtocol::m_stopTime),
                   MakeTimeChecker ())
  ;
  return tid;
}

BundleProtocol::BundleProtocol ()
  : m_node (0),
    m_cla (0),
    m_bpRxBufferPacket (Create<Packet> (0)),
    m_seq (0),
    m_eid ("dtn:none"),
    m_bpRegInfo (),
    m_bpRoutingProtocol (0)
{ 
  NS_LOG_FUNCTION (this);
}

BundleProtocol::~BundleProtocol ()
{ 
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_cla = 0;

  // Clear out the receive buffer and derefence
  m_bpRxBufferPacket->RemoveAtStart (m_bpRxBufferPacket->GetSize ());
  m_bpRxBufferPacket = 0;

  m_bpRoutingProtocol = 0;
  BpSendBundleStore.clear ();
  BpRecvBundleStore.clear ();
  BpRegistration.clear ();
  BpRecvFragMap.clear ();

  // clear the queue by swapping with empty queue
  std::queue<std::vector <uint8_t> > empty;
  std::swap (m_bpRxCborVectorQueue, empty);

}

void 
BundleProtocol::Open (Ptr<Node> node)
{ 
  NS_LOG_FUNCTION (this << " " << node);
  m_node = node;

  // add the convergence layer protocol
  if (m_l4Type == "Tcp")
    {
      Ptr<BpTcpClaProtocol> cla = CreateObject<BpTcpClaProtocol>();
      m_cla = cla;
      m_cla->SetBundleProtocol (this);
    }
  else if (m_l4Type == "Ltp")
    {
      Ptr<BpLtpClaProtocol> cla = CreateObject<BpLtpClaProtocol>();
      m_cla = cla;
      m_cla->SetBundleProtocol (this);
      //m_cla->SetL4Protocol (static_cast<void*>(m_node->GetObject<ns3::ltp::LtpProtocol>()), m_node->GetObject<ns3::ltp::LtpProtocol>()->GetLocalEngineId ()); // LTP requires pointer to the protocol to operate
      //ns3::Ptr<ns3::ltp::LtpProtocol> protocol = m_node->GetObject<ns3::ltp::LtpProtocol>();
      //uint64_t engineId = protocol->GetLocalEngineId ();
      //m_cla->SetL4Protocol (m_node->GetObject<ns3::ltp::LtpProtocol>(), m_node->GetObject<ns3::ltp::LtpProtocol>()->GetLocalEngineId ()); 
      //m_cla->SetL4Protocol (protocol, engineId);//, true);
      m_cla->SetL4Protocol (m_l4Type);
      cla->SetLinkStatusCheckDelay(1);
    }
  else
    {
      NS_FATAL_ERROR ("BundleProtocol::Open (): unknown tranport layer protocol type! " << m_l4Type);   
    }

}

BpEndpointId
BundleProtocol::BuildBpEndpointId (const std::string &scheme, const std::string &ssp)
{ 
  NS_LOG_FUNCTION (this << " " << scheme << " " << ssp);
  BpEndpointId eid (scheme, ssp); 
 
  m_eid = eid;

  return eid;
}

BpEndpointId
BundleProtocol::BuildBpEndpointId (const std::string &uri)
{ 
  NS_LOG_FUNCTION (this << " " << uri);
  BpEndpointId eid (uri);

  m_eid = eid;

  return eid;
}

void 
BundleProtocol::SetBpEndpointId (BpEndpointId eid)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  m_eid = eid;
}

int
BundleProtocol::ExternalRegisterTcp (const BpEndpointId &eid, const double lifetime, const bool state, const InetSocketAddress l4Address)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri () << " Tcp");
  BpRegisterInfo info;
  info.lifetime = lifetime;
  info.state = state;
  info.cla = m_cla;
  int retval = m_cla->SetL4Address (eid, &l4Address);
  if (retval < 0)
  {
    return -1;
  }
  return Register (eid, info);
}

int
BundleProtocol::ExternalRegisterLtp (const BpEndpointId &eid, const double lifetime, const bool state, const uint64_t l4Address)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri () << " Ltp");
  BpRegisterInfo info;
  info.lifetime = lifetime;
  info.state = state;
  info.cla = m_cla;
  int retval = m_cla->SetL4Address (eid, reinterpret_cast<const InetSocketAddress*>(&l4Address));
  if (retval < 0)
  {
    return -1;
  }
  return Register (eid, info);
}
/*
int
BundleProtocol::ExternalRegister (const BpEndpointId &eid, const double lifetime, const bool state, void* l4Address)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri ());

  BpRegisterInfo info;
  info.lifetime = lifetime;
  info.state = state;
  info.cla = m_cla;
  int retval = m_cla->setL4Address (eid, l4Address);
  if (retval < 0)
  {
    return -1;
  }
  return Register (eid, info);
}
*/
int 
BundleProtocol::Register (const BpEndpointId &eid, const struct BpRegisterInfo &info)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.find (eid);
  if (it == BpRegistration.end ())
    {
      // insert a registration of local endpoint id in the registration storage
      BpRegisterInfo rInfo;
      rInfo.lifetime = info.lifetime;
      rInfo.state = info.state;
      rInfo.cla = info.cla;
      BpRegistration.insert (std::pair<BpEndpointId, BpRegisterInfo> (eid, rInfo));

      if (info.state)
        {
          // enable the transport layer to receive packets
          return m_cla->EnableReceive (eid);
        }

      return 0;
    } 
  else
    {
      // duplicate registration
      NS_FATAL_ERROR ("BundleProtocol::Register (): duplicate registration with endpoint id ");
      return -1;
    }
}

int 
BundleProtocol::Unregister (const BpEndpointId &eid)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.end ();
  it = BpRegistration.find (eid);
  if (it == BpRegistration.end ())
    {
      return -1;
    } 

  (*it).second.state = false;
  return m_cla->DisableReceive (eid);
}

int
BundleProtocol::Bind (const BpEndpointId &eid)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.end ();
  it = BpRegistration.find (eid);
  if (it == BpRegistration.end ())
    {
      return -1;
    } 
  else
    {
      // set the registeration of this eid to active
      (*it).second.state = true;

      return m_cla->EnableReceive (eid);
    }
}

// Current implementation to send data
int
BundleProtocol::Send_data (const uint8_t* data, const uint32_t data_size, const BpEndpointId &src, const BpEndpointId &dst)
{
  NS_LOG_FUNCTION (this << " " << src.Uri () << " " << dst.Uri ());
  // check the source eid is registered or not
  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.end ();
  it = BpRegistration.find (src);
  if (it == BpRegistration.end ())
  {
    // the local eid is not registered
    return -1;
  } 
  else
  {
    // TBD: the lifetime of the eid is expired?
  }


  
  uint32_t total = data_size;
  uint32_t offset = 0;
  bool fragment =  ( total > m_bundleSize ) ? true : false; // TO DO: Move fragmentation to CLA level - only CLA can make appropriate fragmentation decision based on transmitting protocol
  std::time_t timeStamp = std::time (NULL) - RFC_DATE_2000;
  uint8_t crcType = 1; // CRC-16

  NS_LOG_FUNCTION("Received PDU of size: " << total << "; max bundle size is: " << m_bundleSize << (( total > m_bundleSize ) ? "Fragmenting" : "No Fragmenting"));

  // a simple fragementation: ensure a bundle is transmittd by one packet at the transport layer

  while ( total > 0 )   
    { 
      uint32_t copySize = std::min (total, m_bundleSize);
    
      Ptr<BpBundle> bundle = CreateObject<BpBundle> (0, src, dst, copySize, reinterpret_cast<const uint8_t*>(data + offset));
      bundle->GetPrimaryBlockPtr ()->SetDestinationEid (dst);
      bundle->GetPrimaryBlockPtr ()->SetSourceEid (src);
      bundle->GetPrimaryBlockPtr ()->SetLifetime (0);
      bundle->GetPrimaryBlockPtr ()->SetAduLength (data_size); // set the ADU length to the original data size
      bundle->GetPrimaryBlockPtr ()->SetCreationTimestamp (timeStamp); // ensure each fragment maintains the same creation timestamp
      bundle->GetPrimaryBlockPtr ()->SetCrcType (crcType); // set the CRC type to CRC-16

      bundle->GetPayloadBlockPtr ()->SetBlockTypeCode (1);
      bundle->GetPayloadBlockPtr ()->SetBlockNumber (1);
      bundle->GetPayloadBlockPtr ()->SetCrcType (crcType); // set the CRC type to CRC-16

      if (fragment)
        {
          bundle->GetPrimaryBlockPtr ()->SetIsFragment (true);
          bundle->GetPrimaryBlockPtr ()->SetFragmentOffset (offset);
          NS_LOG_FUNCTION ("Current fragment offset: " << offset << ". Current fragment size: " << copySize);
        }
      else
        {
          bundle->GetPrimaryBlockPtr ()->SetIsFragment (false);
        }

      // Assemble the full bundle
      //bundle->RebuildBundle ();

      NS_LOG_FUNCTION ("Send bundle:" << " src eid " << bundle->GetPrimaryBlockPtr ()->GetSourceEid ().Uri () << 
                                 " dst eid " << bundle->GetPrimaryBlockPtr ()->GetDestinationEid ().Uri () << 
                                 " payload size " << bundle->GetPayloadBlockPtr ()->GetBlockDataSize () <<
                                 " serialized bundle size " << bundle->GetCborEncodingSize ());

      // Generate the CRC values
      bundle->GetPrimaryBlockPtr ()->GenerateCrcValue ();
      bundle->GetPayloadBlockPtr ()->GenerateCrcValue ();

      // Reprime bundle with updated values
      bundle->RebuildBundle ();
      
      // store the bundle into persistant sent storage
      std::map<BpEndpointId, std::queue<Ptr<BpBundle> > >::iterator it = BpSendBundleStore.end ();
      it = BpSendBundleStore.find (src);
      if ( it == BpSendBundleStore.end ())
        {
          // this is the first packet sent by this source endpoint id
          std::queue<Ptr<BpBundle> > qu;
          //qu.push (packet);
          qu.push (bundle);
          BpSendBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<BpBundle> > > (src, qu) );
        }
      else
        {
          // ongoing packet
          (*it).second.push (bundle);
        }

      if (m_cla)
        {
           m_cla->SendBundle (bundle);
        }
      else
        NS_FATAL_ERROR ("BundleProtocol::Send (): undefined m_cla");

      total = total - copySize;
      offset = offset + copySize;                             
    }

  return 0;

}

int
BundleProtocol::ForwardBundle (Ptr<BpBundle> bundle)
{
  NS_LOG_FUNCTION (this << " " << bundle);
  BpPrimaryBlock *bpPrimaryblock = bundle->GetPrimaryBlockPtr();
  BpEndpointId src = bpPrimaryblock->GetSourceEid ();

  // process extension blocks
  ProcessExtensionBlocks (bundle);

  // store the bundle into persistant sent storage
  std::map<BpEndpointId, std::queue<Ptr<BpBundle> > >::iterator it = BpSendBundleStore.end ();
  it = BpSendBundleStore.find (src);
  if ( it == BpSendBundleStore.end ())
    {
      // this is the first packet sent by this source endpoint id
      std::queue<Ptr<BpBundle> > qu;
      qu.push (bundle);
      BpSendBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<BpBundle> > > (src, qu) );
    }
  else
    {
      // ongoing packet
      (*it).second.push (bundle);
    }

  if (m_cla)
    {
        m_cla->SendBundle (bundle);                             
    }
  else
  {
    NS_FATAL_ERROR ("BundleProtocol::ForwardBundle (): undefined m_cla");
    return -1;
  }
  return 0;
}

int
BundleProtocol::Close (const BpEndpointId &eid)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.end ();
  it = BpRegistration.find (eid);
  if (it == BpRegistration.end ())
    {
      return -1;
    } 
  else
    {
      // TBD: call cla to close transport layer connection

      BpRegistration.erase (it);
    }

  return 0;
}

void
BundleProtocol::RetreiveCBORBundle ()
{ 
  NS_LOG_FUNCTION (this);
  /*
  uint32_t size = m_bpRxBufferPacket->GetSize ();
  NS_LOG_FUNCTION (this << " Received packet of size: " << size);
  int size_check = 180;
  if (size > size_check)
    {
      
      BpBundleHeader bundleHeader;
      m_bpRxBufferPacket->PeekHeader (bundleHeader);
      uint32_t cborBundleSize = bundleHeader.GetBundleSize();
      if (size < cborBundleSize)
      {
        // the bundle is not complete
        NS_LOG_DEBUG (this << " Retrieved packet size is smaller than declared size.  Will not process and wait for further transmissions.");
        return;
      }
      NS_LOG_FUNCTION (this << " Recieved bundle of size: " << cborBundleSize);
      m_bpRxBufferPacket->RemoveHeader (bundleHeader);
      uint8_t buffer[cborBundleSize];
      m_bpRxBufferPacket->CopyData (&buffer[0], cborBundleSize);
      std::vector <uint8_t> v_buffer (buffer, buffer + cborBundleSize);

      // convert bundle from CBOR to JSON and continue to process
      Ptr<BpBundle> bundle = CreateObject<BpBundle> ();
      
      bundle->SetBundleFromCbor (v_buffer);

      m_bpRxBufferPacket->RemoveAtStart (cborBundleSize);
      NS_LOG_FUNCTION (this << " Retrieved packet.  Will process and check for more.");
      ProcessBundle (bundle); // now process the bundle and perform follow-on actions
      Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this);
    }
  else
    {
      NS_LOG_DEBUG (this << " Retrieved packet size is: " << std::to_string(m_bpRxBufferPacket->GetSize ()) << "; less than " << std::to_string(size_check) << ". Skipping and waiting for additional data.");
    }
    */
   if (!m_bpRxCborVectorQueue.empty ())
   {
      Ptr<BpBundle> bundle = CreateObject<BpBundle> ();
      std::vector <uint8_t> v_buffer = m_bpRxCborVectorQueue.front ();
      m_bpRxCborVectorQueue.pop ();
      int retVal = bundle->SetBundleFromCbor (v_buffer);
      if (retVal < 0)
      {
        NS_LOG_FUNCTION (this << " Error processing bundle.  Dropping");
        return;
      }
      ProcessBundle (bundle); // now process the bundle and perform follow-on actions
      Simulator::ScheduleNow (&BundleProtocol::RetreiveCBORBundle, this); // see if there are any additional bundles to process
   }
}

void
BundleProtocol::RetreiveBundle ()
{
  NS_LOG_FUNCTION (this);
  if (!m_bpRxBundleQueue.empty ())
  {
    Ptr<BpBundle> bundle = m_bpRxBundleQueue.front ();
    m_bpRxBundleQueue.pop ();
    if (!bundle->GetPrimaryBlockPtr ()->CheckCrcValue ())
    {
      NS_LOG_FUNCTION (this << " Primary Block CRC check failed.  Dropping bundle");
      return;
    }
    //bundle->GetPayloadBlockPtr ()->DumpAllButPayload ();
    if (!bundle->GetPayloadBlockPtr ()->CheckCrcValue ())
    {
      NS_LOG_FUNCTION (this << " Payload Block CRC check failed.  Dropping bundle");
      return;
    }
    ProcessBundle (bundle); // now process the bundle and perform follow-on actions
    Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this); // see if there are any additional bundles to process
  }
}

void 
BundleProtocol::ReceivePacket (Ptr<Packet> packet)
{ 
  NS_LOG_FUNCTION (this << " " << packet);
  // add packets into receive buffer
  m_bpRxBufferPacket->AddAtEnd (packet);

  uint32_t packetSize = m_bpRxBufferPacket->GetSize ();
  NS_LOG_FUNCTION (this << " Received packet of size: " << packetSize);
  uint32_t size_check = 180;
  if (packetSize > size_check)
  {  
    BpBundleHeader bundleHeader;
    m_bpRxBufferPacket->PeekHeader (bundleHeader);
    uint32_t declaredCborBundleSize = bundleHeader.GetBundleSize();
    if (packetSize < declaredCborBundleSize)
    {
      // the bundle is not complete
      NS_LOG_FUNCTION (this << " Retrieved packet size is smaller than declared size.  Will not process and wait for further transmissions.");
      return;
    }
    NS_LOG_FUNCTION (this << " Recieved bundle of size: " << declaredCborBundleSize);
    m_bpRxBufferPacket->RemoveHeader (bundleHeader);
    uint8_t buffer[declaredCborBundleSize];
    m_bpRxBufferPacket->CopyData (&buffer[0], declaredCborBundleSize);
    std::vector <uint8_t> v_buffer (buffer, buffer + declaredCborBundleSize);
    m_bpRxCborVectorQueue.push (v_buffer);
    m_bpRxBufferPacket->RemoveAtStart (declaredCborBundleSize);
    Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this);
    //uint32_t packetSize = m_bpRxBufferPacket->GetSize ();
  }
}

void
BundleProtocol::ReceiveCborVector (std::vector <uint8_t> v_bundle)
{ 
  NS_LOG_FUNCTION (this << " Received vector of " << v_bundle.size () << " bytes");
  m_bpRxCborVectorQueue.push (v_bundle);
  Simulator::ScheduleNow (&BundleProtocol::RetreiveCBORBundle, this);
}

void
BundleProtocol::ReceiveBundle (Ptr<BpBundle> bundle)
{ 
  NS_LOG_FUNCTION (this << " " << bundle);
  m_bpRxBundleQueue.push (bundle);
  Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this);
}

void 
BundleProtocol::ProcessBundle (Ptr<BpBundle> bundle)
{ 
  NS_LOG_FUNCTION (this << " " << bundle);

  BpPrimaryBlock* bpPrimaryBlockPtr = bundle->GetPrimaryBlockPtr ();

  BpEndpointId dst = bpPrimaryBlockPtr->GetDestinationEid ();
  BpEndpointId src = bpPrimaryBlockPtr->GetSourceEid ();

  NS_LOG_FUNCTION ("Recv bundle: "<< " src eid " << src.Uri () << 
                                 " dst eid " << dst.Uri ());

  // the destination endpoint eid is registered? 
  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.find (dst);
  if (it == BpRegistration.end ())
    {
      NS_LOG_FUNCTION ("Attempting to process bundle for eid: " << dst.Uri() << " which is not registered with current registration.  Dropping");
      // the destination endpoint id is not registered, drop packet
      return;
    } 
  else
    {
      // TBD: the lifetime of the eid is expired?
    }
  if (dst != this->m_eid)
  {
    // destination is registered and not the current node, so forwarding
    NS_LOG_FUNCTION ("Received bundle for eid: " << dst.Uri() << ". Current eid is: " << this->m_eid.Uri() << ". Forwarding bundle");
    if (ForwardBundle(bundle) < 0)
    {
      NS_LOG_FUNCTION ("Forwarding of bundle to eid: " << dst.Uri() << " failed.  Bundle dropped");
    }
    return;
  }
  // check if this is part of a fragment
  if (bpPrimaryBlockPtr->IsFragment ())
  {
    // store all needed data from headers before we strip from them from fragments
    time_t CreateTimeStamp = bpPrimaryBlockPtr->GetCreationTimestamp ();
    u_int32_t AduLength = bpPrimaryBlockPtr->GetAduLength ();
    u_int32_t FragOffset = bpPrimaryBlockPtr->GetFragmentOffset ();
    
    NS_LOG_FUNCTION (this << "Bundle is part of fragment: timestamp=" << CreateTimeStamp <<
                              "total ADU length: " << AduLength <<
                              " offset=" << FragOffset);

    
    // Build a unique identifier for this fragment
    std::string FragName = src.Uri () + "_" + std::to_string(CreateTimeStamp);
    // is this the first fragment of the bundle we've received?
    std::map<std::string, std::map<u_int32_t, Ptr<BpBundle> > >::iterator itBpFrag = BpRecvFragMap.find (FragName);
    if (itBpFrag == BpRecvFragMap.end ())
    {
      // this is the first fragment of this bundle received
      NS_LOG_FUNCTION (this << " First fragment for bundle: " << FragName);
      // Once all the fragment sizes add up to total ADU size, we can reconstruct based on fragment offset
      std::map<u_int32_t, Ptr<BpBundle> > FragMap;
      FragMap.insert (std::pair<u_int32_t, Ptr<BpBundle> > (FragOffset, bundle));
      BpRecvFragMap.insert (std::pair<std::string, std::map<u_int32_t, Ptr<BpBundle> > > (FragName, FragMap));
    }
    else
    {
      // some bundle fragments already received
      NS_LOG_FUNCTION (this << " Already have some fragments, adding offset: " << FragOffset);
      std::map<u_int32_t, Ptr<BpBundle> >::iterator itFrag = (*itBpFrag).second.find(FragOffset);
      if (itFrag != (*itBpFrag).second.end ())
      {
        NS_LOG_FUNCTION (this << " Bundle fragment already received. Dropping");
        return;
      }
      (*itBpFrag).second.insert (std::pair<u_int32_t, Ptr<BpBundle> > (FragOffset, bundle));
    }
    // test if bundle is now complete
    NS_LOG_FUNCTION (this << " Checking for complete bundle");
    itBpFrag = BpRecvFragMap.find (FragName); // Retrieving the fragmentation map again in the event of a first fragment received
    u_int32_t CurrentBundleLength = 0;
    
    BpCanonicalBlock* fragBpPayloadBlockPtr;
    for (auto& it : (*itBpFrag).second)
    {
      NS_LOG_FUNCTION (this << "inspecting fragment: " << it.second);
      fragBpPayloadBlockPtr = it.second->GetPayloadBlockPtr ();
      u_int32_t fragBpPayloadLength = fragBpPayloadBlockPtr->GetBlockDataSize ();
      CurrentBundleLength += fragBpPayloadLength;
      NS_LOG_FUNCTION (this << "Found fragment with block data size: " << fragBpPayloadLength << ". CurrentBundleLength: " << CurrentBundleLength);
    }
    if (CurrentBundleLength != AduLength)
    {
      // we do not have the complete bundle. Return and wait for rest of bundle fragments to come in
      NS_LOG_FUNCTION (this << " Have " << CurrentBundleLength << " out of " << AduLength << ". Waiting to receive rest");
      return;
    }
    // bundle is complete, begine assembly
    NS_LOG_FUNCTION (this << " Have complete bundle of size " << CurrentBundleLength << ". Reassembling");
    // Get first fragment and start building from there;

    u_int32_t CurrFragOffset = 0;
    u_int32_t fragBpPayloadLength = 0;
    CurrentBundleLength = 0;
    std::string fragmentBuffer = "";
    bool corruptedBundle = false;

    for (; CurrentBundleLength < AduLength; CurrFragOffset = CurrentBundleLength)
    {
      std::map<u_int32_t, Ptr<BpBundle> >::iterator itFrag = (*itBpFrag).second.find(CurrFragOffset);
      if (itFrag == (*itBpFrag).second.end ())
      {
        NS_LOG_FUNCTION (this << " Could not find fragment with offset: " << CurrFragOffset);
        corruptedBundle = true;
        break;
      }
      
      fragBpPayloadBlockPtr = itFrag->second->GetPayloadBlockPtr ();
      if (fragBpPayloadBlockPtr == nullptr)
      {
        NS_LOG_FUNCTION (this << " Bundle fragment missing payload block. Inserting notification into ADU");
        corruptedBundle = true;
        break;
      }
      fragBpPayloadLength = fragBpPayloadBlockPtr->GetBlockDataSize ();
      NS_LOG_FUNCTION (this << " Found fragment with offset: " << CurrFragOffset << " with size " << fragBpPayloadLength);
      std::string fragBpPayload = fragBpPayloadBlockPtr->GetBlockData ();
      fragmentBuffer += fragBpPayload;
      CurrentBundleLength += fragBpPayloadLength;
    }
    if (corruptedBundle)
    {
      fragmentBuffer = "Corrupted bundle: Missing fragments; no ADU available";
    }
    // fragmentBuffer now contains the reconstructed ADU
    NS_LOG_FUNCTION ( this << " Reconstructed ADU: " << FragName << " with size " << fragmentBuffer.size ());
    BpCanonicalBlock* payloadBlockPtr = bundle->GetPayloadBlockPtr ();
    payloadBlockPtr->SetBlockData (fragmentBuffer);  // replaced bundle ADU with reconstructed ADU
    bpPrimaryBlockPtr->SetIsFragment (false);
    // Now have reconstructed packet, delete fragment map and erase from BpRecvFragMap
    (*itBpFrag).second.clear ();
    BpRecvFragMap.erase (FragName);
  }
  
  // Check for extension blocks
  ProcessExtensionBlocks (bundle, true); // true indicates we are only printing the extension block contents for informative purposes only

  std::map<BpEndpointId, std::queue<Ptr<BpBundle> > >::iterator itMap = BpRecvBundleStore.find (dst);

  if ( itMap == BpRecvBundleStore.end ())
  {
    // this is the first bundle received by this destination endpoint id
    NS_LOG_FUNCTION (this << " First bundle received for dst: " << dst.Uri () << "; creating new queue");
    std::queue<Ptr<BpBundle> > qu;
    qu.push (bundle);
    BpRecvBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<BpBundle> > > (dst, qu) );
  }
  else
  {
    // ongoing bundles
    NS_LOG_FUNCTION (this << " Have previously received bundle for dst: " << dst.Uri () << "; storing in existing queue");
    (*itMap).second.push (bundle);
  }
  // Notify application that bundle has been received
  NotifyBundleRecv ();
}

std::vector<uint8_t>
BundleProtocol::Receive_data (const BpEndpointId &eid)
{
  NS_LOG_FUNCTION (this << " " << eid.Uri ());

  Ptr<BpBundle> bundle = Receive (eid);
  if (bundle == NULL)
    {
      // no bundles for this eid
      return std::vector<uint8_t> ();  // return empty vector
    }
  else
    {
      // get the payload block
      BpCanonicalBlock *payloadBlockPtr = bundle->GetPayloadBlockPtr ();
      std::string payloadData = payloadBlockPtr->GetBlockData ();
      std::vector<uint8_t> data (payloadData.begin(), payloadData.end());
      return data;
    }
}

// Which BpEndpointId is this using?  Why does it need one?  Should I have a separete function that just receives any?
Ptr<BpBundle>
BundleProtocol::Receive (const BpEndpointId &eid)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  Ptr<BpBundle> emptyBundle = NULL;

  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.find (eid);
  if (it == BpRegistration.end ())
    {
      // the eid is not registered
      NS_LOG_FUNCTION (this << "The eid is not registered: "<< eid.Uri());
      return emptyBundle;
    } 
  else
    {
      // TBD: the lifetime of the eid is expired?
     
      // return all the bundles with dst eid = eid
      std::map<BpEndpointId, std::queue<Ptr<BpBundle> > >::iterator itMap = BpRecvBundleStore.end ();
      itMap = BpRecvBundleStore.find (eid);
      if ( itMap == BpRecvBundleStore.end ())
      {
        // do not receive any bundle with this dst eid
        NS_LOG_FUNCTION (this << "Did not receive any bundle with this dst eid: "<< eid.Uri());
        return emptyBundle;
      }
      else if ( (*itMap).second.size () > 0)
        {
          Ptr<BpBundle> bundle = ((*itMap).second).front ();
          ((*itMap).second).pop ();   
          return bundle;
        }
      else
        {
          // has already fetched all bundles with this dst eid
          BpRecvBundleStore.erase (itMap);
          return emptyBundle;
        }
    }
}

BpEndpointId 
BundleProtocol::GetBpEndpointId () const
{ 
  NS_LOG_FUNCTION (this);
  return m_eid;
}

void
BundleProtocol::SetRoutingProtocol (Ptr<BpRoutingProtocol> route)
{ 
  NS_LOG_FUNCTION (this << " " << route);
  m_cla->SetRoutingProtocol (route);
}

Ptr<BpRoutingProtocol> 
BundleProtocol::GetRoutingProtocol ()
{ 
  NS_LOG_FUNCTION (this);
  return m_cla->GetRoutingProtocol ();
}

Ptr<Node> 
BundleProtocol::GetNode () const
{ 
  NS_LOG_FUNCTION (this);
  return m_node;
}

Ptr<BpClaProtocol>
BundleProtocol::GetCla ()
{
  NS_LOG_FUNCTION (this);
  return m_cla;
}

Ptr<BpBundle> 
BundleProtocol::GetBundle (const BpEndpointId &src)
{ 
  NS_LOG_FUNCTION (this << " " << src.Uri ());
  std::map<BpEndpointId, std::queue<Ptr<BpBundle> > >::iterator it = BpSendBundleStore.find (src); 
  if ( it == BpSendBundleStore.end ())
    {
      NS_LOG_FUNCTION (this << " Did not find bundle in SendBundleStore with uri: " << src.Uri ());
      return NULL;
    }
  else
    {
      if ( ((*it).second).size () == 0)
      {
        NS_LOG_FUNCTION (this << " Found queue SendBundleStore for uri: " << src.Uri () << " but is empty");
        return NULL;
      }
      Ptr<BpBundle> bundle = ((*it).second).front ();
      ((*it).second).pop ();

      return bundle;
    }
}


void
BundleProtocol::ProcessExtensionBlocks (Ptr<BpBundle> bundle, bool printOnly)
{
  NS_LOG_FUNCTION (this << " " << bundle);

  if (printOnly)
  {
    if (bundle->HasExtensionBlock(BpCanonicalBlock::BLOCK_TYPE_PREVIOUS_NODE))
    {
      std::string prevNodeStr = bundle->GetPrevNodeExtBlockData ();
      NS_LOG_FUNCTION (this << " Bundle has previous node of: " << prevNodeStr);
    }
    if (bundle->HasExtensionBlock(BpCanonicalBlock::BLOCK_TYPE_BUNDLE_AGE))
    {
      uint64_t bundleAge = bundle->GetBundleAgeExtBlockData ();
      NS_LOG_FUNCTION (this << " Bundle has age of: " << bundleAge << " milliseconds");
    }
    if (bundle->HasExtensionBlock(BpCanonicalBlock::BLOCK_TYPE_HOP_COUNT))
    {
      uint64_t hopCount = bundle->GetHopCountExtBlockData ();
      NS_LOG_FUNCTION (this << " Bundle has hop count of: " << hopCount);
    }
  }
  else
  {
    // Apply extension bundles for forwarding per RFC 9171
    bundle->AddUpdatePrevNodeExtBlock (m_eid);
    bundle->AddUpdateBundleAgeExtBlock ();
    bundle->AddUpdateHopCountExtBlock ();
    // TODO:  Implement method to delete bundle if extension block thresholds have been reached
  }
}

void
BundleProtocol::SetRecvCallback (Callback<void, Ptr<BundleProtocol> > receivedBundleCb)
{
  NS_LOG_FUNCTION (this << &receivedBundleCb);
  m_receivedBundleCb = receivedBundleCb;
}

void
BundleProtocol::NotifyBundleRecv ()
{
  NS_LOG_FUNCTION (this);
  if (!m_receivedBundleCb.IsNull ())
  {
    m_receivedBundleCb (this);
  }
}

void 
BundleProtocol::SetBpRegisterInfo (struct BpRegisterInfo info)
{ 
  NS_LOG_FUNCTION (this);
  m_bpRegInfo = info;
}

void 
BundleProtocol::DoInitialize (void)
{ 
  NS_LOG_FUNCTION (this);
  m_startEvent = Simulator::Schedule (m_startTime, &BundleProtocol::StartBundleProtocol, this);
  if (m_stopTime != TimeStep (0))
    {
      m_stopEvent = Simulator::Schedule (m_stopTime, &BundleProtocol::StopBundleProtocol, this);
    }
  Object::DoInitialize ();
}

void
BundleProtocol::StartBundleProtocol ()
{ 
  NS_LOG_FUNCTION (this);
  if (!m_cla)
    NS_FATAL_ERROR (this << " BundleProtocol::DoInitialize (): do not define convergence layer! ");   

  if ( m_cla->GetRoutingProtocol () == 0)
    NS_FATAL_ERROR (this << " BundleProtocol::DoInitialize (): do not define bundle routing protocol! ");   

  Register (m_eid, m_bpRegInfo);
}

void
BundleProtocol::StopBundleProtocol ()
{ 
  NS_LOG_FUNCTION (this);
  Close (m_eid);
}


void
BundleProtocol::SetStartTime (Time start)
{ 
  NS_LOG_FUNCTION (this << " " << start.GetSeconds ());
  m_startTime = start;
}

void
BundleProtocol::SetStopTime (Time stop)
{ 
  NS_LOG_FUNCTION (this << stop.GetSeconds ());
  m_stopTime = stop;
}

void
BundleProtocol::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_cla = 0;
  m_bpRoutingProtocol = 0;
  m_receivedBundleCb = MakeNullCallback<void, Ptr<BundleProtocol> > ();
  m_startEvent.Cancel ();
  m_stopEvent.Cancel ();
  Object::DoDispose ();
}


} // namespace ns3

