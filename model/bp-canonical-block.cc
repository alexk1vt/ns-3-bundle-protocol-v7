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
#include "bp-canonical-block.h"
#include <stdio.h>
#include <string>

#include <vector>
#include <ctime>

#define RFC_DATE_2000 946684800

NS_LOG_COMPONENT_DEFINE ("BpCanonicalBlock");

namespace ns3 {

/* Public */
BpCanonicalBlock::BpCanonicalBlock ()
  : m_blockTypeCode (0),
    m_blockNumber (0),
    m_blockProcessingFlags (0),
    m_crcType (0),
    m_crcValue (0),
    m_blockData ("")
{
    NS_LOG_FUNCTION (this);
    m_canonical_block["block_type_code"] = m_blockTypeCode;
    m_canonical_block["block_number"] = m_blockNumber;
    m_canonical_block["block_processing_flags"] = m_blockProcessingFlags;
    m_canonical_block["crc_type"] = m_crcType;
    // m_canonical_block["crc_value"] = m_crcValue; //TODO:  calculate CRC value
    m_canonical_block["block_data"] = m_blockData;
}

BpCanonicalBlock::BpCanonicalBlock (uint8_t code, uint32_t number, uint64_t flags, uint8_t crcType, std::string data)
  : m_blockTypeCode (code),
    m_blockNumber (number),
    m_blockProcessingFlags (flags),
    m_crcType (crcType),
    m_blockData (data)
{
    NS_LOG_FUNCTION (this);
    m_canonical_block["block_type_code"] = m_blockTypeCode;
    m_canonical_block["block_number"] = m_blockNumber;
    m_canonical_block["block_processing_flags"] = m_blockProcessingFlags;
    m_canonical_block["crc_type"] = m_crcType;
    // m_canonical_block["crc_value"] = m_crcValue; //TODO:  calculate CRC value
    m_canonical_block["block_data"] = m_blockData;
}

BpCanonicalBlock::~BpCanonicalBlock ()
{
    NS_LOG_FUNCTION (this);
}

void
BpCanonicalBlock::SetBlockTypeCode (uint8_t code)
{
    NS_LOG_FUNCTION (this << (int) code);
    m_blockTypeCode = code;
}

void
BpCanonicalBlock::SetBlockNumber (uint32_t number)
{
    NS_LOG_FUNCTION (this << number);
    m_blockNumber = number;
}

void
BpCanonicalBlock::SetBlockProcessingFlags (uint64_t flags)
{
    NS_LOG_FUNCTION (this << flags);
    m_blockProcessingFlags = flags;
}

void
BpCanonicalBlock::SetCrcType (uint8_t crcType)
{
    NS_LOG_FUNCTION (this << (int) crcType);
    m_crcType = crcType;
}

void
BpCanonicalBlock::SetBlockData (std::string data)
{
    NS_LOG_FUNCTION (this << data);
    m_blockData = data;
}

void
BpCanonicalBlock::RebuildCanonicalBlock ()
{
    NS_LOG_FUNCTION (this);
    m_canonical_block["block_type_code"] = m_blockTypeCode;
    m_canonical_block["block_number"] = m_blockNumber;
    m_canonical_block["block_processing_flags"] = m_blockProcessingFlags;
    m_canonical_block["crc_type"] = m_crcType;
    // m_canonical_block["crc_value"] = m_crcValue; //TODO:  calculate CRC value
    m_canonical_block["block_data"] = m_blockData;
}

uint8_t
BpCanonicalBlock::GetBlockTypeCode () const
{
    NS_LOG_FUNCTION (this);
    return m_blockTypeCode;
}

uint32_t
BpCanonicalBlock::GetBlockNumber () const
{
    NS_LOG_FUNCTION (this);
    return m_blockNumber;
}

uint64_t
BpCanonicalBlock::GetBlockProcessingFlags () const
{
    NS_LOG_FUNCTION (this);
    return m_blockProcessingFlags;
}

uint8_t
BpCanonicalBlock::GetCrcType () const
{
    NS_LOG_FUNCTION (this);
    return m_crcType;
}

uint32_t
BpCanonicalBlock::GetCrcValue () const;
{
    NS_LOG_FUNCTION (this);
    return m_crcValue;
}

std::string
BpCanonicalBlock::GetBlockData () const
{
    NS_LOG_FUNCTION (this);
    return m_blockData;
}

uint32_t
BpCanonicalBlock::GetBlockDataSize () const
{
    NS_LOG_FUNCTION (this);
    return m_blockData.size ();
}

json
BpCanonicalBlock::GetJson () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block;
}

uint32_t
BpCanonicalBlock::CalcCrcValue () const
{
    NS_LOG_FUNCTION (this);
    std::vector <std::uint8_t> m_canonical_block_cbor_encoding = json.to_cbor(m_canonical_block); // CBOR encoding of the primary bundle block -- needed for CRC calculation
    // remove CRC value from field (if present) (replace with zeros), then encode block in CBOR
    // calculate CRC value based on CRC type from CBOR encoding
    // restore original CrcValue to field and return newly calculated CRC value
}

} // namespace ns3
