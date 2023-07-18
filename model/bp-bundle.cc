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
 * Author: Gerard Garcia <ggarcia@deic.uab.cat>
 * 
 * BP-7 Author: Alexander Kedrowitsch <alexk1@vt.edu>
 */

#include "ns3/log.h"
#include "ns3/node.h"
#include "bp-bundle.h"
#include <stdio.h>
#include <string>

#include <vector>
#include <ctime>

#define RFC_DATE_2000 946684800

NS_LOG_COMPONENT_DEFINE ("BpBundle");

namespace ns3 {

/* Public */

// :: General BpBundle Functions Not Part of Class ::

void
PrintVectorBuffer (std::vector <std::uint8_t> v_cbor)
{
    NS_LOG_FUNCTION ("::PrintVectorBuffer::");
    uint elementCount = v_cbor.size();
    std::cout << "  ::PrintVectorBuffer:: CBOR with " << elementCount << " elements, each " << sizeof(uint8_t) << "bytes in size. Output: ";
    for (uint32_t i = 0; i < elementCount; i++)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint8_t>(v_cbor[i]) << " ";
    }
    std::cout << std::endl;
}

void
CompareCborBytesToVector (std::vector <std::uint8_t> v_cbor1, std::vector <std::uint8_t> v_cbor2)
{
    NS_LOG_FUNCTION ("::CompareCborBytesToVector::");
    uint v_cbor1_ElementCount = v_cbor1.size();
    uint v_cbor2_ElementCount = v_cbor2.size();
    if (v_cbor1_ElementCount != v_cbor2_ElementCount)
    {
        NS_LOG_FUNCTION ("  ::CompareCborBytesToVector:: Vector sizes are NOT equal - v_cbor1: " << v_cbor1_ElementCount << ", v_cbor2_ElementCount: " << v_cbor2_ElementCount);
        return;
    }
    NS_LOG_FUNCTION ("  ::CompareCborBytesToVector:: Both vectors report element count of: " << v_cbor1_ElementCount);
    for (uint i = 0; i < v_cbor1_ElementCount; i++)
    {
        if (v_cbor1[i] != v_cbor2[i])
        {
            NS_LOG_FUNCTION ("  ::CompareCborBytesToVector:: Element mismatch at index: " << i);
            return;
        }
    }
    NS_LOG_FUNCTION ("  ::CompareCborBytesToVector:: Both vectors match!");
}

void
PrintCborBytes (std::vector <std::uint8_t> cborBundle)
{
    NS_LOG_FUNCTION ("::PrintCborBytes::");
    uint elementCount = cborBundle.size();
    std::cout << "  ::PrintCborBytes:: CBOR with " << elementCount << " elements, each " << sizeof(uint8_t) << "bytes in size. Output: ";
    for (uint32_t i = 0; i < elementCount; i++)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint8_t>(cborBundle[i]) << " ";
    }
    std::cout << std::endl;
}

// :: BpBundleHeader Class Functions ::

BpBundleHeader::BpBundleHeader ()
    : m_bundleSize (0)
{
    NS_LOG_FUNCTION (this);
}

BpBundleHeader::~BpBundleHeader ()
{
  NS_LOG_FUNCTION (this);
}

void
BpBundleHeader::SetBundleSize (const uint32_t size)
{
    NS_LOG_FUNCTION (this);
    m_bundleSize = size;
}

TypeId
BpBundleHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BpBundleHeader")
                      .SetParent<Header> ()
                      .AddConstructor<BpBundleHeader> ();

  return tid;
}

TypeId
BpBundleHeader::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

uint32_t 
BpBundleHeader::GetSerializedSize (void) const
{
    NS_LOG_FUNCTION (this);
    return 4;
}

void
BpBundleHeader::Serialize (Buffer::Iterator start) const
{
    NS_LOG_FUNCTION (this);
    Buffer::Iterator i = start;

    i.WriteU32(m_bundleSize);
    return;
}

uint32_t
BpBundleHeader::Deserialize (Buffer::Iterator start)
{
    NS_LOG_FUNCTION (this);
    Buffer::Iterator i = start;

    m_bundleSize = i.ReadU32();
    return GetSerializedSize ();
}

void
BpBundleHeader::Print (std::ostream &os) const
{
    NS_LOG_FUNCTION (this);
    os << "Bundle Size: " << m_bundleSize << std::endl;
}

uint32_t
BpBundleHeader::GetBundleSize () const
{
    NS_LOG_FUNCTION (this);
    return m_bundleSize;
}

BpBundle::BpBundle ()
  : m_retentionConstraint (0)
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
}

BpBundle::BpBundle (uint8_t constraint, BpPrimaryBlock primaryBlock, BpCanonicalBlock payloadBlock)
    : m_retentionConstraint (constraint)
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
    m_bundle["primary_block"] = primaryBlock.GetJson();
    m_bundle["payload_block"] = payloadBlock.GetJson();
}

BpBundle::BpBundle (uint8_t constraint, BpEndpointId src, BpEndpointId dst, uint32_t payloadSize, const uint8_t *payloadData)
    : m_retentionConstraint (constraint)
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
    m_primaryBlock = BpPrimaryBlock(src, dst, payloadSize);
    m_payloadBlock = BpCanonicalBlock(1, payloadSize, payloadData);
    m_bundle["primary_block"] = m_primaryBlock.GetJson();
    m_bundle["payload_block"] = m_payloadBlock.GetJson();
}

BpBundle::BpBundle (std::vector <uint8_t> cborBundle)
{
    NS_LOG_FUNCTION (this);
    this->SetBundleFromCbor (cborBundle);    
}

BpBundle::~BpBundle ()
{
    NS_LOG_FUNCTION (this);
}

bool
BpBundle::IsDispatchPending () const
{
    NS_LOG_FUNCTION (this);
    BundleRetentionConstraint constraint = BUNDLE_DISPATCH_PENDING;
    return m_retentionConstraint & constraint;
}

bool
BpBundle::IsForwardPending () const
{
    NS_LOG_FUNCTION (this);
    BundleRetentionConstraint constraint = BUNDLE_FORWARD_PENDING;
    return m_retentionConstraint & constraint;
}

bool
BpBundle::IsReassemblyPending () const
{
    NS_LOG_FUNCTION (this);
    BundleRetentionConstraint constraint = BUNDLE_REASSEMBLY_PENDING;
    return m_retentionConstraint & constraint;
}

uint8_t
BpBundle::GetRetentionConstraint () const
{
    NS_LOG_FUNCTION (this);
    return m_retentionConstraint;
}

BpPrimaryBlock
BpBundle::GetPrimaryBlock () const
{
    NS_LOG_FUNCTION (this);
    return m_primaryBlock;
}

BpCanonicalBlock
BpBundle::GetPayloadBlock () const
{
    NS_LOG_FUNCTION (this);
    return m_payloadBlock;
}

BpCanonicalBlock
BpBundle::GetExtensionBlock () const
{
    NS_LOG_FUNCTION (this);
    return m_extensionBlock;  // TODO:  implement support for multiple extension blocks
}

uint8_t
BpBundle::GetExtensionBlockCount () const
{
    NS_LOG_FUNCTION (this);
    return 0;       // TODO:  correct this once support for multiple extension blocks is implemented
}

std::vector <std::uint8_t>
BpBundle::GetCborEncoding ()
{
    NS_LOG_FUNCTION (this);
    m_bundle_cbor_encoding = json::to_cbor(m_bundle);
    //PrintCborBytes(m_bundle_cbor_encoding);
    return m_bundle_cbor_encoding;
}

uint
BpBundle::GetCborEncodingSize ()
{
    NS_LOG_FUNCTION (this);
    uint  elementCount = m_bundle_cbor_encoding.size();
    if (elementCount == 0)
    {
        elementCount = GetCborEncoding().size();
    }
    return elementCount * sizeof(uint8_t);
}

void
BpBundle::SetRetentionConstraint (uint8_t constraint)
{
    NS_LOG_FUNCTION (this << static_cast<uint16_t> (constraint));
    m_retentionConstraint = constraint;
    m_bundle["retention_constraint"] = m_retentionConstraint;
}

void
BpBundle::SetPrimaryBlock (const BpPrimaryBlock &primaryBlock)
{
    NS_LOG_FUNCTION (this);
    m_primaryBlock = primaryBlock;
    m_bundle["primary_block"] = m_primaryBlock.GetJson();
}

void
BpBundle::SetPayloadBlock (const BpCanonicalBlock &payloadBlock)
{
    NS_LOG_FUNCTION (this);
    m_payloadBlock = payloadBlock;
    m_bundle["payload_block"] = m_payloadBlock.GetJson();
}

void
BpBundle::SetExtensionBlock (const BpCanonicalBlock &extensionBlock)
{
    NS_LOG_FUNCTION (this);
    m_extensionBlock = extensionBlock;
    m_bundle["extension_block"] = m_extensionBlock.GetJson();
}

void
BpBundle::RebuildBundle ()
{
    NS_LOG_FUNCTION (this);
    m_primaryBlock.RebuildBlock();
    m_payloadBlock.RebuildBlock();

    // rebuild the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
    m_bundle["primary_block"] = m_primaryBlock.GetJson();
    m_bundle["payload_block"] = m_payloadBlock.GetJson();

    if (GetExtensionBlockCount() > 0)
    {
        m_extensionBlock.RebuildBlock();
        m_bundle["extension_block"] = m_extensionBlock.GetJson(); // TODO - implement support for multiple extension blocks
    }

    this->GetCborEncoding ();
}

void
BpBundle::SetBundleFromCbor (std::vector <std::uint8_t> cborBundle)
{
    NS_LOG_FUNCTION (this);
    //PrintCborBytes (cborBundle);
    m_bundle = json::from_cbor(cborBundle);
    m_retentionConstraint = m_bundle["retention_constraint"];
    m_primaryBlock.SetPrimaryBlockFromJson (m_bundle["primary_block"]);
    m_payloadBlock.SetCanonicalBlockFromJson (m_bundle["payload_block"]);
    if (GetExtensionBlockCount() > 0)
    {
        m_extensionBlock.SetCanonicalBlockFromJson (m_bundle["extension_block"]); // TODO - implement support for multiple extension blocks
    }
}

void
BpBundle::PrintCborBytes ()
{
    NS_LOG_FUNCTION (this);
    uint elementCount = m_bundle_cbor_encoding.size();
    uint cborsize = this->GetCborEncodingSize();
    std::cout << "CBOR with " << elementCount << " elements, each " << sizeof(uint8_t) << "bytes in size. GetCborEncodingSize(): " << cborsize << ", Output: ";
    for (uint32_t i = 0; i < elementCount; i++)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint8_t>(m_bundle_cbor_encoding[i]) << " ";
    }
    std::cout << std::endl;
}

} // namespace ns3
