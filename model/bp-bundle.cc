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
}

BpBundle::BpBundle (uint8_t constraint, BpPrimaryBlock primaryBlock, BpCanonicalBlock payloadBlock)
    : m_retentionConstraint (constraint)
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_bundle[BUNDLE_PRIMARY_BLOCK] = primaryBlock.GetJson();
    m_bundle[BUNDLE_PAYLOAD_BLOCK] = payloadBlock.GetJson();
}

BpBundle::BpBundle (uint8_t constraint, BpEndpointId src, BpEndpointId dst, uint32_t payloadSize, const uint8_t *payloadData)
    : m_retentionConstraint (constraint)
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_primaryBlock = BpPrimaryBlock(src, dst, payloadSize);
    m_payloadBlock = BpCanonicalBlock(BpCanonicalBlock::BLOCK_TYPE_PAYLOAD, 1, 0, 0, payloadSize, payloadData);
    m_bundle[BUNDLE_PRIMARY_BLOCK] = m_primaryBlock.GetJson();
    m_bundle[BUNDLE_PAYLOAD_BLOCK] = m_payloadBlock.GetJson();
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
BpBundle::GetExtensionBlock (uint8_t blockType)
{
    NS_LOG_FUNCTION (this << blockType);

    for (json::iterator it = begin (); it != end (); ++it)
    {
        if (it.value ().is_null ())
        {
            continue;
        }
        if (it.key () == BUNDLE_PRIMARY_BLOCK)
        {
            continue;
        }
        if (it.value () [CANONICAL_BLOCK_FIELD_TYPE_CODE] == blockType)
        {
            NS_LOG_FUNCTION (this << " Found extension block of type: " << blockType);
            uint8_t blockNumber = it.value () [CANONICAL_BLOCK_FIELD_BLOCK_NUMBER];
            uint64_t blockProcessingFlags = it.value () [CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS];
            uint8_t crcType = it.value () [CANONICAL_BLOCK_FIELD_CRC_TYPE];
            std::string blockData = it.value () [CANONICAL_BLOCK_FIELD_BLOCK_DATA].get<std::string> ();
            BpCanonicalBlock extBlock (blockType, blockNumber, blockProcessingFlags, crcType, blockData);
            return extBlock;
        }
    }
    return BpCanonicalBlock ();
}

uint8_t
BpBundle::GetExtensionBlockCount ()
{
    NS_LOG_FUNCTION (this);
    uint8_t extBlockCnt = 0;
    for (json::iterator it = begin (); it != end (); ++it)
    {
        if (it.value ().is_null ())
        {
            continue;
        }
        if (it.key () == BUNDLE_PRIMARY_BLOCK)
        {
            continue;
        }
        if (it.value () [CANONICAL_BLOCK_FIELD_TYPE_CODE] == BpCanonicalBlock::BLOCK_TYPE_PAYLOAD)
        {
            continue;
        }
        extBlockCnt++;
    }

    return extBlockCnt;       // TODO:  correct this once support for multiple extension blocks is implemented
}

std::vector <std::uint8_t>
BpBundle::GetCborEncoding ()
{
    NS_LOG_FUNCTION (this);
    m_bundle_cbor_encoding = json::to_cbor(m_bundle);
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
}

void
BpBundle::SetPrimaryBlock (const BpPrimaryBlock &primaryBlock)
{
    NS_LOG_FUNCTION (this);
    m_primaryBlock = primaryBlock;
    m_bundle[BUNDLE_PRIMARY_BLOCK] = m_primaryBlock.GetJson();
}

void
BpBundle::SetPayloadBlock (const BpCanonicalBlock &payloadBlock)
{
    NS_LOG_FUNCTION (this);
    m_payloadBlock = payloadBlock;
    m_bundle[BUNDLE_PAYLOAD_BLOCK] = m_payloadBlock.GetJson();
}

int
BpBundle::AddExtensionBlock (const BpCanonicalBlock &extensionBlock)
{
    NS_LOG_FUNCTION (this);
    m_extensionBlock = extensionBlock;
    uint8_t blockNumber = GetNewBlockNumber ();
    std::string blockNumberStr = BlockNumberToString(blockNumber);
    m_extensionBlock.SetBlockNumber(blockNumber);
    try
    {
        m_bundle[blockNumberStr] = m_extensionBlock.GetJson();
    }
    catch (const std::exception& e)
    {
        NS_LOG_FUNCTION ("  ::AddBundleBlocksToBundle:: Exception caught: " << e.what());
        return -1;
    }
    return 0;
}

int
BpBundle::AddUpdateExtensionBlock (uint8_t blockType, BpEndpointId eid)
{
  NS_LOG_FUNCTION (this << " " << blockType);
  if (blockType == BpCanonicalBlock::BLOCK_TYPE_PREVIOUS_NODE)
  {
    return AddUpdatePrevNodeExtBlock (eid);
  }
  
  return AddUpdateExtensionBlock (blockType);
}

int
BpBundle::AddUpdateExtensionBlock (uint8_t blockType)
{
  NS_LOG_FUNCTION (this << " " << blockType);
  if (blockType == BpCanonicalBlock::BLOCK_TYPE_PREVIOUS_NODE)
  {
    NS_LOG_FUNCTION (this << " Unable to add/update previous node extension block without endpoint id");
    return -1;
  }
  else if (blockType == BpCanonicalBlock::BLOCK_TYPE_BUNDLE_AGE)
  {
    return AddUpdateBundleAgeExtBlock ();
  }
  else if (blockType == BpCanonicalBlock::BLOCK_TYPE_HOP_COUNT)
  {
    return AddUpdateHopCountExtBlock ();
  }
  else
  {
    NS_LOG_FUNCTION (this << " " << blockType << " is unknown extension block type");
    return -1;
  }
}

int
BpBundle::AddUpdateExtensionBlock (const BpCanonicalBlock &extensionBlock)
{
    NS_LOG_FUNCTION (this);
    uint8_t blockType = extensionBlock.GetBlockTypeCode();
    json::iterator it = m_bundle.begin ();
    for (; it != m_bundle.end (); ++it)
    {
        if (it.value ().is_null () || it.key () == BUNDLE_PRIMARY_BLOCK) // skipping first block as primary blocks don't have block number fields
        {
            continue;
        }
        if (it.value ()[CANONICAL_BLOCK_FIELD_TYPE_CODE] == blockType)
        {
            try
            {
                NS_LOG_FUNCTION (this << " Extension block of type: " << blockType << " already present in bundle. Updating.");
                m_bundle[it.key ()] = extensionBlock.GetJson();
            }
            catch (const std::exception& e)
            {
                NS_LOG_FUNCTION ("  ::AddUpdateExtensionBlock:: Exception caught: " << e.what());
                return -1;
            }
            return 0;
        }
    }
    if (it == m_bundle.end ())
    {
        NS_LOG_FUNCTION (this << " Extension block of type: " << blockType << " not present in bundle. Adding.");
        AddExtensionBlock(extensionBlock);
        return 0;
    }
    return -1;

}

int
BpBundle::AddUpdateExtensionBlock (uint8_t blockType, uint64_t blockProcessingFlags, uint8_t crcType, std::string blockData)
{
    NS_LOG_FUNCTION (this << blockType);

    json::iterator it = m_bundle.begin ();
    for (; it != m_bundle.end (); ++it) 
    {
        if (it.value ().is_null () || it.key () == BUNDLE_PRIMARY_BLOCK) // skipping first block as primary blocks don't have block number fields
        {
            continue;
        }
        if (it.value ()[CANONICAL_BLOCK_FIELD_TYPE_CODE] == blockType)
        {
            // bundle already has extension block of this type code, so update it
            try
            {
                NS_LOG_FUNCTION(this << " Extension block of type " << blockType << " already present in bundle. Updating values.");
                uint32_t blockNumber = std::stoul(it.key ());
                BpCanonicalBlock newExtBlock = BpCanonicalBlock(blockType, blockNumber, blockProcessingFlags, crcType, blockData);
                m_bundle[it.key ()] = newExtBlock.GetJson();
            }
            catch (const std::exception& e)
            {
                NS_LOG_FUNCTION ("  ::AddUpdateExtBundle:: Exception caught: " << e.what());
                return -1;
            }
            return 0;
        }
    }
    if (it == m_bundle.end ())
    {
        // bundle does not have extension block of this type code, so add it

        NS_LOG_FUNCTION(this << " Extension block of type " << blockType << " not present in bundle. Inserting new block.");
        uint8_t blockNumber = GetNewBlockNumber ();
        BpCanonicalBlock newExtBlock = BpCanonicalBlock(blockType, blockNumber, blockProcessingFlags, crcType, blockData);
        AddExtensionBlock(newExtBlock);
        return 0;
    }
    return -1;
}

int
BpBundle::AddUpdatePrevNodeExtBlock (BpEndpointId eid)
{
  NS_LOG_FUNCTION (this);
  uint8_t blockType = BpCanonicalBlock::BLOCK_TYPE_PREVIOUS_NODE;
  uint64_t blockProcessingFlags = 0;
  uint8_t crcType = 0;
  std::string blockData = eid.Uri (); // set previous node as the current node
  return AddUpdateExtensionBlock (blockType, blockProcessingFlags, crcType, blockData);
}

std::string
BpBundle::GetPrevNodeExtBlockData ()
{
  NS_LOG_FUNCTION (this);
  BpCanonicalBlock prevNodeExtBlock = GetExtensionBlock (BpCanonicalBlock::BLOCK_TYPE_PREVIOUS_NODE);
  std::string prevNodeStr = prevNodeExtBlock.GetBlockData ();
  return prevNodeStr;
}

int
BpBundle::AddUpdateBundleAgeExtBlock ()
{
  NS_LOG_FUNCTION (this);
  uint8_t blockType = BpCanonicalBlock::BLOCK_TYPE_BUNDLE_AGE;
  uint64_t blockProcessingFlags = 0;
  uint8_t crcType = 0;
  std::string blockData = "";
  std::time_t creationTimestamp = GetPrimaryBlockPtr ()->GetCreationTimestamp ();
  std::time_t currentTimestamp = std::time (NULL) - RFC_DATE_2000;
  uint64_t age = currentTimestamp - creationTimestamp; // elapsed time in seconds
  age = age * 1000; // elapsed time in milliseconds
  
  // convert age to string
  std::string byteString(sizeof(uint64_t), '\0');
  std::memcpy(&byteString[0], &age, sizeof(uint64_t)); // NOTE - this breaks json .dump() function as non-UTF8 data is stored in the string
  blockData = byteString;
  return AddUpdateExtensionBlock (blockType, blockProcessingFlags, crcType, blockData);
}

uint64_t
BpBundle::GetBundleAgeExtBlockData ()
{
  NS_LOG_FUNCTION (this);
  BpCanonicalBlock bundleAgeExtBlock = GetExtensionBlock (BpCanonicalBlock::BLOCK_TYPE_BUNDLE_AGE);
  std::string blockData = bundleAgeExtBlock.GetBlockData ();
  uint64_t age = 0;
  std::memcpy(&age, &blockData[0], sizeof(uint64_t));
  return age;
}

int
BpBundle::AddUpdateHopCountExtBlock ()
{
  NS_LOG_FUNCTION (this);
  uint64_t hopCount = GetHopCountExtBlockData (); // returns 0 if no previous hop count extension block
  uint8_t blockType = BpCanonicalBlock::BLOCK_TYPE_HOP_COUNT;
  uint64_t blockProcessingFlags = 0;
  uint8_t crcType = 0;
  std::string blockData = "";
  hopCount++;
  std::string byteString (sizeof(uint64_t), '\0');
  std::memcpy(&byteString[0], &hopCount, sizeof(uint64_t));  // NOTE - this breaks json .dump() function as non-UTF8 data is stored in the string
  blockData = byteString;
  return AddUpdateExtensionBlock (blockType, blockProcessingFlags, crcType, blockData);
}

uint64_t
BpBundle::GetHopCountExtBlockData ()
{
  NS_LOG_FUNCTION (this);
  BpCanonicalBlock hopCountExtBlock = GetExtensionBlock (BpCanonicalBlock::BLOCK_TYPE_HOP_COUNT);
  if (hopCountExtBlock.IsEmpty ())
  {
    return 0;
  }
  std::string blockData = hopCountExtBlock.GetBlockData ();
  uint64_t hopCount = 0;
  std::memcpy(&hopCount, &blockData[0], sizeof(uint64_t));  
  return hopCount;
}

bool
BpBundle::HasExtensionBlock (uint8_t blockType)
{
    NS_LOG_FUNCTION (this << " " << blockType);

    for (json::iterator it = begin (); it != end (); ++it)
    {
        if (it.value ().is_null () || it.key () == BUNDLE_PRIMARY_BLOCK) // skipping first block as primary blocks don't have block number fields
        {
            continue;
        }
        if (it.value ()[CANONICAL_BLOCK_FIELD_TYPE_CODE] == blockType)
        {
            return true;
        }
    }
    return false;
}

int
BpBundle::AddBlocksFromBundle(Ptr<BpBundle> donatingBundle)
{
    NS_LOG_FUNCTION (this);
    
    json::iterator it = donatingBundle->begin ();
    for (; it != donatingBundle->end (); ++it)
    {
        try
        {
            m_bundle[it.key ()] = it.value ();
        }
        catch (const std::exception& e)
        {
            NS_LOG_FUNCTION ("  ::AddBundleBlocksToBundle:: Exception caught: " << e.what());
            return -1;
        }
    }
    return 0;
}

int
BpBundle::AddBlocksFromBundle(Ptr<BpBundle> donatingBundle, BpCanonicalBlock::CanonicalBlockTypeCodes blockType, bool headlessBundle)
{
    NS_LOG_FUNCTION (this << blockType);

    if (blockType == BpCanonicalBlock::BLOCK_TYPE_PRIMARY) // primary blocks don't have block type fields, so need to check for this seperate
    {
        try
        {
            SetPrimaryBlock(donatingBundle->GetPrimaryBlock());
        }
        catch (const std::exception& e)
        {
            NS_LOG_FUNCTION ("  ::AddBundleBlocksToBundle:: Exception caught: " << e.what());
            return -1;
        }
        return 0;
    }

    json::iterator it = donatingBundle->begin ();
    if (!headlessBundle)
    {
        if (it == donatingBundle->end ())
        {
            return -1;
        }
        ++it; // skip the primary block
    }
    for (; it != donatingBundle->end (); ++it)
    {
        if (it.value ().is_null ())
        {
            continue;
        }
        if (it.value ()[CANONICAL_BLOCK_FIELD_TYPE_CODE] == blockType)
        {
            try
            {
                if (blockType == BpCanonicalBlock::BLOCK_TYPE_PAYLOAD)
                {
                    SetPayloadBlock(donatingBundle->GetPayloadBlock());
                }
                else
                {
                    BpCanonicalBlock extBlock;
                    extBlock.SetCanonicalBlockFromJson (it.value ());
                    AddExtensionBlock(extBlock);
                }
            }
            catch (const std::exception& e)
            {
                NS_LOG_FUNCTION ("  ::AddBundleBlocksToBundle:: Exception caught: " << e.what());
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

int
BpBundle::AddBlocksFromBundleExcept(Ptr<BpBundle> donatingBundle, BpCanonicalBlock::CanonicalBlockTypeCodes blockType, bool headlessBundle)
{
    NS_LOG_FUNCTION (this << blockType);

    if (blockType != BpCanonicalBlock::BLOCK_TYPE_PRIMARY) // primary blocks don't have block type fields, so need to check for this seperate
    {
        try
        {
            m_bundle[BpBundle::BUNDLE_PRIMARY_BLOCK] = donatingBundle->m_bundle[BpBundle::BUNDLE_PRIMARY_BLOCK];
        }
        catch (const std::exception& e)
        {
            NS_LOG_FUNCTION ("  ::AddBundleBlocksToBundleExcept:: Exception caught: " << e.what());
            return -1;
        }
    }

    json::iterator it = donatingBundle->begin ();
    if (!headlessBundle)
    {
        if (it == donatingBundle->end ())
        {
            return -1;
        }
        ++it; // skip the primary block
    }
    for (; it != donatingBundle->end (); ++it)
    {
        if (it.value ().is_null ())
        {
            continue;
        }
        BpCanonicalBlock::CanonicalBlockTypeCodes itBlockType = it.value ()[CANONICAL_BLOCK_FIELD_TYPE_CODE];
        if (itBlockType != blockType)
        {
            try
            {
                if (itBlockType == BpCanonicalBlock::BLOCK_TYPE_PAYLOAD)
                {
                    m_bundle[BUNDLE_PAYLOAD_BLOCK] = it.value ();
                }
                else
                {
                    uint8_t blockNumber = GetNewBlockNumber ();
                    std::string blockNumberStr = BlockNumberToString(blockNumber);
                    m_bundle[blockNumberStr] = it.value ();
                    m_bundle[blockNumberStr][CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = blockNumber;
                }
            }
            catch (const std::exception& e)
            {
                NS_LOG_FUNCTION ("  ::AddBundleBlocksToBundleExcept:: Exception caught: " << e.what());
                return -1;
            }
        }
    }
    return 0;
}

int
BpBundle::AddToBundle(uint8_t blockIndex, BpCanonicalBlock block)
{
    NS_LOG_FUNCTION (this);

    try
    {
        m_bundle[blockIndex] = block.GetJson(); 
    }
    catch(const std::exception& e)
    {
        NS_LOG_FUNCTION ("  ::AddToBundle:: Exception caught: " << e.what());
        return -1;
    }
    return 0;
}

int
BpBundle::SetBundleFromCbor (std::vector <std::uint8_t> cborBundle)
{
    NS_LOG_FUNCTION (this);
    json tempBundle;

    try
    {
        tempBundle = json::from_cbor(cborBundle);
    } catch (int exceptionValue) {
        NS_LOG_FUNCTION ("  ::SetBundleFromCbor:: Unable to convert bundle from CBOR.  Exception caught: " << exceptionValue);
        return -1;
    }
    return SetBundleFromJson (tempBundle);
}

int
BpBundle::SetBundleFromJson (json donatingJson)
{
    NS_LOG_FUNCTION (this << " donatingJson size: " << donatingJson.size ());
    m_retentionConstraint = 0;

    json::iterator it = donatingJson.begin ();
    for (; it != donatingJson.end (); ++it)
    {
        if (it.value ().is_null ())
        {
            continue;
        }
        if (it.key () == BUNDLE_PRIMARY_BLOCK) // primary block
        {
            m_primaryBlock.SetPrimaryBlockFromJson (it.value ());
            m_bundle[BUNDLE_PRIMARY_BLOCK] = m_primaryBlock.GetJson ();
        }
        else if (it.key () == BUNDLE_PAYLOAD_BLOCK) // payload block
        {
            m_payloadBlock.SetCanonicalBlockFromJson (it.value ());
            m_bundle[BUNDLE_PAYLOAD_BLOCK] = m_payloadBlock.GetJson ();
        }
        else // extension block
        {
            uint8_t blockNumber = GetNewBlockNumber ();
            it.value ()[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = blockNumber;
            m_extensionBlock.SetCanonicalBlockFromJson (it.value ());
            m_bundle[BlockNumberToString(blockNumber)] = m_extensionBlock.GetJson ();
        }
    }
    return 0;
}

uint8_t
BpBundle::GetNewBlockNumber ()
{
    NS_LOG_FUNCTION (this);
    uint8_t highestBlockNumber = 1;
    json::iterator it = m_bundle.begin ();
    for (; it != m_bundle.end (); ++it)
    {
        if (it.value ().contains(CANONICAL_BLOCK_FIELD_BLOCK_NUMBER) && it.value ()[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] > highestBlockNumber)
        {
            highestBlockNumber = it.value ()[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER];
        }
    }
    return highestBlockNumber + 1;
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

std::string
BpBundle::BlockNumberToString (uint8_t blockNumber)
{
    NS_LOG_FUNCTION (this);
    std::string rawNumberStr = std::to_string(blockNumber);
    std::string formattedNumberStr = "000" + rawNumberStr;
    formattedNumberStr = formattedNumberStr.substr(formattedNumberStr.length() - 3);
    return formattedNumberStr;
}

bool
BpBundle::empty () const
{
    NS_LOG_FUNCTION (this);
    return m_bundle.empty();
}

json::iterator
BpBundle::begin()
{
    NS_LOG_FUNCTION (this);
    return m_bundle.begin();
}

json::iterator
BpBundle::end()
{
    NS_LOG_FUNCTION (this);
    return m_bundle.end ();
}

} // namespace ns3
