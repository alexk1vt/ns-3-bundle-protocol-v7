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
#include "bundle-protocol.h"
//#include "bp-header.h"
//#include "bp-payload-header.h"
#include "bp-primary-block.h"
#include "bp-canonical-block.h"
#include <algorithm>
#include <map>
#include <ctime>

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
BundleProtocol::ExternalRegister (const BpEndpointId &eid, const double lifetime, const bool state, const InetSocketAddress l4Address)
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

int
BundleProtocol::Send_file (std::string file_path, const BpEndpointId &src, const BpEndpointId &dst)
{
  // reference value of m_bundleSize for fragmenting
  // can I just read the file and store into an NS-3 packet, then call Send_packet?  what's the upper-limit on NS-3 packet sizes?

  // Otherwise I'll need to build my own header and such.  Work that should be performed by underlying layers...
  return 0;
}

/*
* NOTE:  Send(..) will be replaced by contents of Send_packet(..) once it has been fully vetted.
* Do not rely on /reference this code for future development - reference Send_packet(..) instead
*/
int 
BundleProtocol::Send (Ptr<Packet> p, const BpEndpointId &src, const BpEndpointId &dst)
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

  uint32_t total = p->GetSize ();
  bool fragment =  ( total > m_bundleSize ) ? true : false;

  // a simple fragementation: ensure a bundle is transmittd by one packet at the transport layer
  uint32_t num = 0;
  while ( total > 0 )   
    { 
      Ptr<Packet> packet = NULL;
      uint32_t size = p->GetSize();;

      // build bundle payload header
      BpPayloadHeader bpph;

      // build primary bundle header
      BpHeader bph;
      bph.SetDestinationEid (dst);
      bph.SetSourceEid (src);

      bph.SetCreateTimestamp (std::time(NULL));
      bph.SetSequenceNumber (m_seq);
      m_seq++;

      size = std::min (total, m_bundleSize);

      bph.SetBlockLength (size);       
      bph.SetLifeTime (0);

      if (fragment)
        {
          bph.SetIsFragment (true);
          bph.SetFragOffset (p->GetSize () - size);
          bph.SetAduLength (p->GetSize ());
        }
      else
        {
          bph.SetIsFragment (false);
          bph.SetAduLength (p->GetSize ());
        }

      bph.SetBlockLength (size);       
      bpph.SetBlockLength (size);

      packet = Create<Packet> (size);

      packet->AddHeader (bpph);
      packet->AddHeader (bph);

      NS_LOG_DEBUG ("Send bundle:" << " seq " << bph.GetSequenceNumber ().GetValue () << 
                                 " src eid " << bph.GetSourceEid ().Uri () << 
                                 " dst eid " << bph.GetDestinationEid ().Uri () << 
                                 " pkt size " << packet->GetSize ());


      // store the bundle into persistant sent storage
      std::map<BpEndpointId, std::queue<Ptr<Packet> > >::iterator it = BpSendBundleStore.end ();
      it = BpSendBundleStore.find (src);
      if ( it == BpSendBundleStore.end ())
        {
          // this is the first packet sent by this source endpoint id
          std::queue<Ptr<Packet> > qu;
          qu.push (packet);
          BpSendBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<Packet> > > (src, qu) );
        }
      else
        {
          // ongoing packet
          (*it).second.push (packet);
        }

      if (m_cla)
        {
           //m_cla->SendPacket (packet);
           if (m_cla->SendPacket (packet) == 0)
           {
             num++;
           }
           else
           {
            NS_LOG_FUNCTION(this << " " << "CLA unable to send to send bundle");
           }
        }
      else
        NS_FATAL_ERROR ("BundleProtocol::Send (): undefined m_cla");

      total = total - size;

      // force the convergence layer to send the packet
      if (num == 1)
        if (m_cla->SendPacket (packet) != 0)
        {
          NS_LOG_FUNCTION(this << " " << "CLA unable to send to send bundle");
        }
    }

  return 0;
}

// Current implementation to send data
// Packet p constains the ADU to send
int 
BundleProtocol::Send_packet (Ptr<Packet> p, const BpEndpointId &src, const BpEndpointId &dst)
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

  uint32_t total = p->GetSize ();
  uint32_t offset = 0;
  bool fragment =  ( total > m_bundleSize ) ? true : false;

  NS_LOG_FUNCTION("Received PDU of size: " << total << "; max bundle size is: " << m_bundleSize << (( total > m_bundleSize ) ? "Fragmenting" : "No Fragmenting"));

  // a simple fragementation: ensure a bundle is transmittd by one packet at the transport layer
  SequenceNumber32 seqNum (0);

  //std::time_t timestamp = std::time(NULL);

  while ( total > 0 )   
    { 
      // initialize the packet that will form our bundle payload
      Ptr<Packet> packet = NULL;

      // build bundle primary block
      BpPrimaryBlock bpPrimaryblock;

      //// build bundle payload header
      //BpPayloadHeader bpph;
      

      //// build primary bundle header
      //BpHeader bph;
      //bph.SetDestinationEid (dst);
      //bph.SetSourceEid (src);

      //bph.SetCreateTimestamp (timestamp);
      //bph.SetSequenceNumber (seqNum);
    
      bpPrimaryblock.SetDestinationEid (dst);
      bpPrimaryblock.SetSourceEid (src);

      uint32_t size = std::min (total, m_bundleSize);

      bph.SetBlockLength (size);       
      bph.SetLifeTime (0);

      if (fragment)
        {
          //bph.SetIsFragment (true);
          //bph.SetFragOffset (offset);
          //bph.SetAduLength (p->GetSize ()); // ADU length specifies the original ADU size
          bpPrimaryblock.SetIsFragment (true);
          bpPrimaryblock.SetFragmentOffset (offset);
          bpPrimaryblock.SetAduLength (p->GetSize ()); // ADU length specifies the original ADU size
          NS_LOG_FUNCTION ("Current fragment offset: " << offset << ". Current fragment size: " << size);
        }
      else
        {
          //bph.SetIsFragment (false);
          //bph.SetAduLength (p->GetSize ());
          bpPrimaryblock.SetIsFragment (false);
          bpPrimaryblock.SetAduLength (p->GetSize ());
        }

      //bph.SetBlockLength (size);       
      //bpph.SetBlockLength (size);

      // Build the bundle payload block
      // copy application data to packet (either as a whole or as a fragment)
      //packet = p->CreateFragment(offset, size);
      std::string bundlePayload = std::string(p->CreateFragment(offset, size))
      BpCanonicalBlock bpPayloadblock;
      bpPayloadblock.SetBlockTypeCode (1);
      bpPayloadblock.SetBlockNumber (1);
      bpPayloadblock.SetBlockData (bundlePayload);


      // Assemble the full bundle
      bpPrimaryblock.RebuildPrimaryBundleBlock();
      bpPayloadblock.RebuildCanonicalBundleBlock();
      BpBundle bundle;
      bundle.SetPrimaryBlock(bpPrimaryblock);
      bundle.SetPayloadBlock(bpPayloadblock);
      bundle.RebuildBundle();

      // Serialize bundle into CBOR encoding and load into NS-3 packet
      Ptr<Packet> packet(bundle.GetCborEncoding());
      //packet->AddHeader (bpph);
      //packet->AddHeader (bph);

      NS_LOG_FUNCTION ("Send bundle:" << " src eid " << bpPrimaryblock.GetSourceEid ().Uri () << 
                                 " dst eid " << bpPrimaryblock.GetDestinationEid ().Uri () << 
                                 " pkt size " << packet->GetSize ());


      // store the bundle into persistant sent storage
      std::map<BpEndpointId, std::queue<Ptr<Packet> > >::iterator it = BpSendBundleStore.end ();
      it = BpSendBundleStore.find (src);
      if ( it == BpSendBundleStore.end ())
        {
          // this is the first packet sent by this source endpoint id
          std::queue<Ptr<Packet> > qu;
          qu.push (packet);
          BpSendBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<Packet> > > (src, qu) );
        }
      else
        {
          // ongoing packet
          (*it).second.push (packet);
        }

      if (m_cla)
        {
           m_cla->SendPacket (packet);                             
           seqNum++;
        }
      else
        NS_FATAL_ERROR ("BundleProtocol::Send (): undefined m_cla");

      total = total - size;
      offset = offset + size;

      // force the convergence layer to send the packet
      //if (num == 1)
      //  m_cla->SendPacket (packet);                             
    }

  return 0;
}

int
BundleProtocol::ForwardBundle (Ptr<Packet> bundle)
{
  NS_LOG_FUNCTION (this << " " << bundle);
  BpHeader bpHeader;         // primary bundle header
  bundle->PeekHeader (bpHeader);
  BpEndpointId src = bpHeader.GetSourceEid ();

  // store the bundle into persistant sent storage
  std::map<BpEndpointId, std::queue<Ptr<Packet> > >::iterator it = BpSendBundleStore.end ();
  it = BpSendBundleStore.find (src);
  if ( it == BpSendBundleStore.end ())
    {
      // this is the first packet sent by this source endpoint id
      std::queue<Ptr<Packet> > qu;
      qu.push (bundle);
      BpSendBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<Packet> > > (src, qu) );
    }
  else
    {
      // ongoing packet
      (*it).second.push (bundle);
    }

  if (m_cla)
    {
        m_cla->SendPacket (bundle);                             
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
BundleProtocol::RetreiveBundle ()
{ 
  NS_LOG_FUNCTION (this);
  //BpHeader bpHeader;         // primary bundle header
  //BpPayloadHeader bppHeader; // bundle payload header

  // continue to retreive a bundle from buffer until the buffer size is smaller than a bundle or a bundle header 
  // ACS start
  // if (m_bpRxBufferPacket->GetSize () > (bpHeader.GetSerializedSize () + bppHeader.GetSerializedSize ()))
  
  //if (m_bpRxBufferPacket->GetSize () > 180)
  // ACS end
  // AlexK start
  NS_LOG_FUNCTION (this << " Received packet of size: " << m_bpRxBufferPacket->GetSize ());
  int rand_val = 180;
  if (m_bpRxBufferPacket->GetSize () > rand_val)
  // Alexk end
    {
      uint32_t size = m_bpRxBufferPacket->GetSize ();
      std::vector <uint8_t> buffer (size);
      m_bpRxBufferPacket->CopyData (&buffer[0], size);
      // LEFT OFF HERE!!
      // convert bundle from CBOR to JSON and continue to process
      BpBundle bundle;
      bundle.SetBundleFromCbor (buffer);
      BpPrimaryBlock BpPrimaryBlock = bundle.GetPrimaryBlock ();

      // how to know if the entire bundle is in the buffer?
      // examples would be packet level fragmentation or if the buffer is holding more than one bundle
      m_bpRxBufferPacket->RemoveAtStart (size);
      NS_LOG_FUNCTION (this << " Retrieved packet.  Will process and check for more.");
      ProcessBundle (bundle);
      Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this);


      m_bpRxBufferPacket->PeekHeader (bpHeader);
      m_bpRxBufferPacket->PeekHeader (bppHeader);

      uint32_t total =  bpHeader.GetBlockLength ()
                      + bpHeader.GetSerializedSize () 
                      + bppHeader.GetSerializedSize ();
    }
  else
    {
      NS_LOG_DEBUG (this << " Retrieved packet size is: " << std::to_string(m_bpRxBufferPacket->GetSize ()) << "; less than " << std::to_string(rand_val) << " Dropping.");
    }
}
      /*if (m_bpRxBufferPacket->GetSize () >= total)
        {
          Ptr<Packet> bundle = m_bpRxBufferPacket->CreateFragment (0, total) ;
          m_bpRxBufferPacket->RemoveAtStart (total);

          NS_LOG_FUNCTION (this << " Retrieved packet.  Will process and check for more.");
          ProcessBundle (bundle);
          
          // continue to check bundles
          Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this);
        }
      else
        {
          NS_LOG_DEBUG (this << " Retrieved packet size is smaller than declared size.  Dropping?");
        }
    } 
  else
    {
      NS_LOG_DEBUG (this << " Retrieved packet size is: " << std::to_string(m_bpRxBufferPacket->GetSize ()) << "; less than " << std::to_string(rand_val) << " Dropping.");
    }

}*/

void 
BundleProtocol::ReceivePacket (Ptr<Packet> packet)
{ 
  NS_LOG_FUNCTION (this << " " << packet);
  // add packets into receive buffer
  m_bpRxBufferPacket->AddAtEnd (packet);

  // ACS start
  // if (packet->GetSize() > 180)
  // {
  // ACS end
    Simulator::ScheduleNow (&BundleProtocol::RetreiveBundle, this);
  // ACS start
  // }
  // ACS end
}

void 
BundleProtocol::ProcessBundle (BpBundle bundle)
{ 
  NS_LOG_FUNCTION (this << " " << bundle);
  //BpHeader bpHeader;         // primary bundle header
  //BpPayloadHeader bppHeader; // bundle payload header
  BpPrimaryBlock bpPrimaryBlock = bundle->GetPrimaryBlock ();

  //bundle->PeekHeader (bpHeader);
  //bundle->PeekHeader (bppHeader);

  
  //BpEndpointId dst = bpHeader.GetDestinationEid ();
  //BpEndpointId src = bpHeader.GetSourceEid ();
  BpEndpointId dst = bpPrimaryBlock.GetDestinationEid ();
  BpEndpointId src = bpPrimaryBlock.GetSourceEid ();


  //NS_LOG_DEBUG ("Recv bundle:" << " seq " << bpHeader.GetSequenceNumber ().GetValue () << 
  //                            " src eid " << bpHeader.GetSourceEid ().Uri () << 
  //                            " dst eid " << bpHeader.GetDestinationEid ().Uri () << 
  //                            " packet size " << bundle->GetSize ());
  NS_LOG_DEBUG ("Recv bundle: "<< " src eid " << src.Uri () << 
                                 " dst eid " << dst.Uri ())

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
  //if (bpHeader.IsFragment ()){
  if (bpPrimaryBlock.IsFragment ())
  {
    // store all needed data from headers before we strip from them from fragments
    //time_t CreateTimeStamp = bpHeader.GetCreateTimestamp ();
    time_t CreateTimeStamp = bpPrimaryBlock.GetCreateTimestamp ();
    //u_int32_t AduLength = bpHeader.GetAduLength ();
    u_int32_t AduLength = bpPrimaryBlock.GetAduLength ();
    u_int32_t FragOffset = bpPrimaryBlock.GetFragmentOffset ();
    
    NS_LOG_FUNCTION (this << "Bundle is part of fragment: timestamp=" << CreateTimeStamp <<
                              "total ADU length: " << AduLength <<
                              " offset=" << FragOffset);
                              //"seq=" << bpHeader.GetSequenceNumber ().GetValue () <<
                              //" offset=" << bpHeader.GetFragOffset ()); 
    
    // Build a unique identifier for this fragment
    std::string FragName = src.Uri () + "_" + std::to_string(CreateTimeStamp);
    // is this the first fragment of the bundle we've received?
    //std::map<std::string, std::map<u_int32_t, Ptr<Packet> > >::iterator itBpFrag = BpRecvFragMap.find (FragName);
    std::map<std::string, std::map<u_int32_t, BpBundle > >::iterator itBpFrag = BpRecvFragMap.find (FragName);
    if (itBpFrag == BpRecvFragMap.end ())
    {
      // this is the first fragment of this bundle received
      NS_LOG_FUNCTION (this << " First fragment for bundle: " << FragName);
      // LEFT OFF HERE!!
      // Change Fragmap to contain total ADU size as well as the offset and block data size of each fragment
      // Once all the fragment sizes add up to total ADU size, we can reconstruct based on fragment offset
      //std::map<u_int32_t, Ptr<Packet> > FragMap;
      std::map<u_int32_t, BpBundle > FragMap;
      //FragMap.insert (std::pair<u_int32_t, Ptr<Packet> > (bpHeader.GetSequenceNumber ().GetValue (), bundle));
      FragMap.insert (std::pair<u_int32_t, BpBundle > (FragOffset, bundle));
      BpRecvFragMap.insert (std::pair<std::string, std::map<u_int32_t, BpBundle > > (FragName, FragMap));
    }
    else
    {
      // some bundle fragments already received

      //u_int32_t fragSeqNum = bpHeader.GetSequenceNumber ().GetValue ();
      //NS_LOG_FUNCTION (this << " Already have some fragments, adding sequence number: " << fragSeqNum);
      //std::map<u_int32_t, Ptr<Packet> >::iterator itFrag = (*itBpFrag).second.find(fragSeqNum);
      NS_LOG_FUNCTION (this << " Already have some fragments, adding offset: " << FragOffset);
      std::map<u_int32_t, BpBundle >::iterator itFrag = (*itBpFrag).second.find(FragOffset);
      if (itFrag != (*itBpFrag).second.end ())
      {
        NS_LOG_FUNCTION (this << " Bundle fragment already received. Dropping");
        return;
      }
      (*itBpFrag).second.insert (std::pair<u_int32_t, BpBundle > (FragOffset, bundle));
    }
    // test if bundle is now complete
    NS_LOG_FUNCTION (this << " Checking for complete bundle");
    itBpFrag = BpRecvFragMap.find (FragName); // Retrieving the fragmentation map again in the event of a first fragment received
    u_int32_t CurrentBundleLength = 0;
    
    for (auto& it : (*itBpFrag).second)
    {
      NS_LOG_FUNCTION (this << "inspecting fragment: " << it.second);
      //BpHeader fragBpHeader;
      //it.second->PeekHeader (fragBpHeader);
      //CurrentBundleLength += fragBpHeader.GetBlockLength ();
      BpPrimaryBlock fragBpPayloadBlock = it.second->GetPayloadBlock ();
      CurrentBundleLength += fragBpPayloadBlock.GetBlockDataSize ();
      NS_LOG_FUNCTION (this << "CurrentBundleLength: " << CurrentBundleLength);
    }
    if (CurrentBundleLength != AduLength)
    {
      // we do not have the complete bundle. Return and wait for rest of bundle fragments to come in
      NS_LOG_FUNCTION (this << " Have " << CurrentBundleLength << " out of " << AduLength << ". Waiting to receive rest");
      return;
    }
    // bundle is complete
    NS_LOG_FUNCTION (this << " Have complete bundle of size " << CurrentBundleLength);
    // Get first fragment and start building from there;
    
    // WITHOUT SEQUENCE NUMBERS, I NEED TO COMPARE FRAGMENTATION OFFSETS TO RECONSTRUCT THE BUNDLE!!

    //u_int32_t FragSeqNum = 0;
    u_int32_t CurrFragOffset = 0;
    std::string fragmentBuffer = "";
    //std::map<u_int32_t, Ptr<Packet> >::iterator itFrag = (*itBpFrag).second.find(FragSeqNum);
    std::map<u_int32_t, BpBundle >::iterator itFrag = (*itBpFrag).second.find(CurrFragOffset);  // Get the first fragment for this bundle

    BpBundle fragmentBundle = (*itFrag).second;
    //bundle->PeekHeader (bpHeader);
    BpCanonicalBlock *payloadBlockPtr = bundle->GetPayloadBlockPtr ();
    CurrentBundleLength = payloadBlockPtr->GetBlockDataSize ();
    //CurrentBundleLength = bpHeader.GetBlockLength ();
    fragmentBuffer += payloadBlockPtr->GetBlockData ();

    // iterate through fragmentation map finding the next largest offset and add it's fragment to the bundle
    //FragSeqNum++;
    CurrFragOffset = CurrentBundleLength;
    BpBundle bundleFragment;
    //for (; CurrentBundleLength < AduLength; FragSeqNum++)
    for (; CurrentBundleLength < AduLength; CurrFragOffset = CurrentBundleLength)
    {
      //itFrag = (*itBpFrag).second.find(FragSeqNum);
      itFrag = (*itBpFrag).second.find(CurrFragOffset);
      //Ptr<Packet> bundleFragment = (*itFrag).second;
      bundleFragment = (*itFrag).second;
      //bundleFragment->PeekHeader (bpHeader);
      payloadBlockPtr = bundleFragment->GetPayloadBlockPtr ();
      //CurrentBundleLength += bpHeader.GetBlockLength ();
      CurrentBundleLength += payloadBlockPtr->GetBlockDataSize ();
      // strip fragment headers
      //bundleFragment->RemoveHeader (bpHeader);
      //bundleFragment->RemoveHeader (bppHeader);
      //bundle->AddAtEnd(bundleFragment);
      fragmentBuffer += payloadBlockPtr->GetBlockData ();
      // ***!!! LEFT OFF HERE !!!***
    }
    // fragmentBuffer now contains the reconstructed ADU
    payloadBlockPtr->SetBlockData (fragmentBuffer);  // replaced bundle ADU with reconstructed ADU
    bpPrimaryBlock.SetIsFragment (false);
    // Now have reconstructed packet, delete fragment map
    BpRecvFragMap.erase (FragName);
  }

  // CHANGE THIS TO STORE BUNDLES BASED ON SRC - NOT DST!!!!
  // store the bundle into persistant received storage
  std::map<BpEndpointId, std::queue<Ptr<Packet> > >::iterator itMap = BpRecvBundleStore.end ();
  //itMap = BpRecvBundleStore.find (dst);
  itMap = BpRecvBundleStore.find (src);
  if ( itMap == BpRecvBundleStore.end ())
    {
      // this is the first bundle received by this destination endpoint id
      std::queue<Ptr<Packet> > qu;
      qu.push (bundle);
      //BpRecvBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<Packet> > > (dst, qu) );
      BpRecvBundleStore.insert (std::pair<BpEndpointId, std::queue<Ptr<Packet> > > (src, qu) );
    }
  else
    {
      // ongoing bundles
      (*itMap).second.push (bundle);
    }

}

// LEFT OFF HERE!!!
// Replacement Receive function that doesn't not accept any parameters and returns a string of the ADU
// This is the function that should be called by the application
std::string
BundleProtocol::Receive ()
{
  NS_LOG_FUNCTION (this);
  std::string stringBuffer = "";
  bool bundleFound = false;

  for (auto const& iter : BpRecvBundleStore)
  {
    if ((*iter).second.size () > 0) 
    {
      bundleFound = true;
      BpBundle = (*iter).second.front ();
      BpCanonicalBlock *payloadBlockPtr = bundle->GetPayloadBlockPtr ();
      stringBuffer += payloadBlockPtr->GetBlockData ();
      (*iter).second.pop ();
      return stringBuffer;
    }
    else
    {
      // no bundles in this queue
      BpRecvBundleStore.erase (iter);
      return stringBuffer;
    }
  }

}


// Which BpEndpointId is this using?  Why does it need one?  Should I have a separete function that just receives any?
Ptr<Packet>
BundleProtocol::Receive (const BpEndpointId &eid)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  Ptr<Packet> emptyPacket = NULL;

  std::map<BpEndpointId, BpRegisterInfo>::iterator it = BpRegistration.find (eid);
  if (it == BpRegistration.end ())
    {
      // the eid is not registered
      NS_LOG_FUNCTION (this << "The eid is not registered: "<< eid.Uri());
      return emptyPacket;
    } 
  else
    {
      // TBD: the lifetime of the eid is expired?
     
      // return all the bundles with dst eid = eid
      std::map<BpEndpointId, std::queue<Ptr<Packet> > >::iterator itMap = BpRecvBundleStore.end ();
      itMap = BpRecvBundleStore.find (eid);
      if ( itMap == BpRecvBundleStore.end ())
        {
          // do not receive any bundle with this dst eid
          NS_LOG_FUNCTION (this << "Did not receive any bundle with this dst eid: "<< eid.Uri());
          return emptyPacket;
        }
      else if ( (*itMap).second.size () > 0)
        {
          Ptr<Packet> packet = ((*itMap).second).front ();
          ((*itMap).second).pop ();

          // remove bundle header before forwarding to applications
          BpHeader bpHeader;         // primary bundle header
          BpPayloadHeader bppHeader; // bundle payload header
          packet->RemoveHeader (bpHeader);
          packet->RemoveHeader (bppHeader);
    
          return packet;
        }
      else
        {
          // has already fetched all bundles with this dst eid
          BpRecvBundleStore.erase (itMap);
          return emptyPacket;
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

Ptr<Packet> 
BundleProtocol::GetBundle (const BpEndpointId &src)
{ 
  NS_LOG_FUNCTION (this << " " << src.Uri ());
  std::map<BpEndpointId, std::queue<Ptr<Packet> > >::iterator it = BpSendBundleStore.find (src); 
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
      Ptr<Packet> packet = ((*it).second).front ();
      ((*it).second).pop ();

      return packet;
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
  m_startEvent.Cancel ();
  m_stopEvent.Cancel ();
  Object::DoDispose ();
}


} // namespace ns3

