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
//#include "bp-crc.h"
#include <stdio.h>
#include <string>

#include <vector>
#include <ctime>

#define RFC_DATE_2000 946684800

NS_LOG_COMPONENT_DEFINE ("BpCanonicalBlock");

namespace ns3 {

// Canonical Block Field name strings for JSON
std::string CANONICAL_BLOCK_FIELD_TYPE_CODE = "00";
std::string CANONICAL_BLOCK_FIELD_BLOCK_NUMBER = "01";
std::string CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS = "02";
std::string CANONICAL_BLOCK_FIELD_CRC_TYPE = "03";
std::string CANONICAL_BLOCK_FIELD_BLOCK_DATA = "04";
std::string CANONICAL_BLOCK_FIELD_CRC_VALUE = "05";

/* Public */
BpCanonicalBlock::BpCanonicalBlock ()
//  : m_blockTypeCode (0),
//    m_blockNumber (0),
//    m_blockProcessingFlags (0),
//    m_crcType (0),
//    m_crcValue (0),
//    m_blockData ("")
{
    NS_LOG_FUNCTION (this);
    m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] = BLOCK_TYPE_UNSPECIFIED;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = 0;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS] = 0;
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE] = 0;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA] = "";
    // m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_VALUE] = m_crcValue; //TODO:  calculate CRC value
}

BpCanonicalBlock::BpCanonicalBlock (uint8_t code, uint32_t number, uint64_t flags, uint8_t crcType, std::string data)
//  : m_blockTypeCode (code),
//    m_blockNumber (number),
//    m_blockProcessingFlags (flags),
//    m_crcType (crcType),
//    m_blockData (data)
{
    NS_LOG_FUNCTION (this);
    m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] = code;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = number;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS] = flags;
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE] = crcType;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA] = data;
}

BpCanonicalBlock::BpCanonicalBlock (uint8_t code, uint8_t number, uint64_t flags, uint8_t crcType, uint32_t payloadSize, const uint8_t *payloadData)
//  : m_blockTypeCode (code),
//    m_blockNumber (number),
//    m_blockProcessingFlags (flags),
//    m_crcType (crcType),
//    m_blockData ("")
{
    NS_LOG_FUNCTION (this);
    // copy the payload data into the block data
    std::string tempData = "";
    for (int i = 0; i < payloadSize; i++)
    {
        tempData += reinterpret_cast<const unsigned char> (payloadData[i]);
    }
    m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] = code;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = number;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS] = flags;
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE] = crcType;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA] = tempData;
    // m_canonical_block[CANONICAL_BLOCK_CRC_VALUE] = m_crcValue; //TODO:  calculate CRC value
}

BpCanonicalBlock::BpCanonicalBlock(uint8_t blockTypeCode, uint32_t payloadSize, const uint8_t *payloadData)
//    : m_blockTypeCode (blockTypeCode),
//      m_blockNumber (0),
//      m_blockProcessingFlags (0),
//      m_crcType (0),
//      m_crcValue (0),
//      m_blockData ("")
{
    NS_LOG_FUNCTION (this);
    // copy the payload data into the block data
    std::string tempData = "";
    for (int i = 0; i < payloadSize; i++)
    {
        tempData += reinterpret_cast<const unsigned char> (payloadData[i]);
    }
    m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] = blockTypeCode;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = 0;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS] = 0;
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE] = 0;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA] = tempData;
    // m_canonical_block["crc_value"] = m_crcValue; //TODO:  calculate CRC value
}

BpCanonicalBlock::~BpCanonicalBlock ()
{
    NS_LOG_FUNCTION (this);
}

void
BpCanonicalBlock::SetCanonicalBlockFromJson (json jsonCanonicalBlock)
{
    NS_LOG_FUNCTION (this);
    m_canonical_block = jsonCanonicalBlock;
    /*
    m_blockTypeCode = m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE];
    m_blockNumber = m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER];
    m_blockProcessingFlags = m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS];
    m_crcType = m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE];
    m_blockData = m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA].get<std::string> ();
    // m_crcValue = m_canonical_block["crc_value"]; //TODO:  calculate CRC value
    //if (m_crcType != 0) // TODO:  Implement this!
    */
}

void
BpCanonicalBlock::SetBlockTypeCode (uint8_t code)
{
    NS_LOG_FUNCTION (this << (int) code);
    m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] = code;
    //m_blockTypeCode = code;
}

void
BpCanonicalBlock::SetBlockNumber (uint32_t number)
{
    NS_LOG_FUNCTION (this << number);
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = number;
    //m_blockNumber = number;
}

void
BpCanonicalBlock::SetBlockProcessingFlags (uint64_t flags)
{
    NS_LOG_FUNCTION (this << flags);
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS] = flags;
    //m_blockProcessingFlags = flags;
}

void
BpCanonicalBlock::SetCrcType (uint8_t crcType)
{
    NS_LOG_FUNCTION (this << (int) crcType);
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE] = crcType;
    //m_crcType = crcType;
}

void
BpCanonicalBlock::SetCrcValue (uint32_t crc)
{
    NS_LOG_FUNCTION (this << crc);
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_VALUE] = crc;
}

void
BpCanonicalBlock::SetBlockData (std::string data)
{
    NS_LOG_FUNCTION (this << data);
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA] = data;
    //m_blockData = data;
}

/*
void
BpCanonicalBlock::RebuildBlock ()
{
    NS_LOG_FUNCTION (this);
    m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] = m_blockTypeCode;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER] = m_blockNumber;
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS] = m_blockProcessingFlags;
    m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE] = m_crcType;
    // m_canonical_block[CANONICAL_BLOCK_CRC_VALUE] = m_crcValue; //TODO:  calculate CRC value
    m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA] = m_blockData;
}
*/

uint8_t
BpCanonicalBlock::GetBlockTypeCode () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE];
}

uint32_t
BpCanonicalBlock::GetBlockNumber () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_NUMBER];
}

uint64_t
BpCanonicalBlock::GetBlockProcessingFlags () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS];
}

uint8_t
BpCanonicalBlock::GetCrcType () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_TYPE];
}

uint16_t
BpCanonicalBlock::GetCrc16Value () const
{
    NS_LOG_FUNCTION (this);
    if (!m_canonical_block.contains (CANONICAL_BLOCK_FIELD_CRC_VALUE))
    {
        NS_LOG_FUNCTION (this << " CRC value field not present in canonical block");
        return 0;
    }
    return m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_VALUE];
}

uint32_t
BpCanonicalBlock::GetCrc32Value () const
{
    NS_LOG_FUNCTION (this);
    if (!m_canonical_block.contains (CANONICAL_BLOCK_FIELD_CRC_VALUE))
    {
        NS_LOG_FUNCTION (this << " CRC value field not present in canonical block");
        return 0;
    }
    return m_canonical_block[CANONICAL_BLOCK_FIELD_CRC_VALUE];
}

std::string
BpCanonicalBlock::GetBlockData () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA].get<std::string> ();
}

uint32_t
BpCanonicalBlock::GetBlockDataSize () const
{
    NS_LOG_FUNCTION (this);
    //return m_canonical_block["block_data"].size ();
    uint32_t blockDataSize = m_canonical_block[CANONICAL_BLOCK_FIELD_BLOCK_DATA].get<std::string> ().size ();
    NS_LOG_FUNCTION( this << " Block data size: " << blockDataSize << " bytes");
    return blockDataSize;
}

json
BpCanonicalBlock::GetJson () const
{
    NS_LOG_FUNCTION (this);
    return m_canonical_block;
}

void
BpCanonicalBlock::DumpAllButPayload () const
{
    NS_LOG_FUNCTION (this);
    uint16_t crc16Value = 0;
    uint32_t crc32Value = 0;
    uint8_t blockTypeCode = GetBlockTypeCode ();
    uint32_t blockNumber = GetBlockNumber ();
    uint64_t blockProcessingFlags = GetBlockProcessingFlags ();
    uint8_t crcType = GetCrcType ();
    if (crcType == 1)
    {
        uint16_t crc16Value = GetCrc16Value ();
    }
    else if (crcType == 2)
    {
        uint32_t crc32Value = GetCrc32Value ();
    }
    NS_LOG_FUNCTION ("  Block Type Code: " << blockTypeCode);
    NS_LOG_FUNCTION ("  Block Number: " << blockNumber);
    NS_LOG_FUNCTION ("   Block Processing Flags: " << blockProcessingFlags);
    NS_LOG_FUNCTION ("   CRC Type: " << (int) crcType);
    if (crcType == 1)
    {
        NS_LOG_FUNCTION ("  CRC Value: " << crc16Value);
    }
    else if (crcType == 2)
    {
        NS_LOG_FUNCTION ("  CRC Value: " << crc32Value);
    }
}

int
BpCanonicalBlock::GenerateCrcValue ()
{
    NS_LOG_FUNCTION (this);
    uint8_t crcType = GetCrcType ();
    NS_LOG_FUNCTION (this << " CRC type: " << crcType);
    if (crcType == 0)
    {
        SetCrcValue (0);
        return 0;
    }
    else if (crcType == 1)
    {
        uint16_t crcValue = CalcCrc16Value ();
        if (crcValue == 0)
        {
            NS_LOG_FUNCTION (this << " CRC calculation error");
            return -1;
        }
        SetCrcValue (crcValue);
    }
    else if (crcType == 2)
    {
        uint32_t crcValue = CalcCrc32Value ();
        if (crcValue == 0)
        {
            NS_LOG_FUNCTION (this << " CRC calculation error");
            return -1;
        }
        SetCrcValue (crcValue);
    }
    else
    {
        NS_LOG_FUNCTION (this << " CRC type not supported");
        return -1;
    }
    return 0;
    
}

uint16_t
BpCanonicalBlock::CalcCrc16Value (void)
{
    NS_LOG_FUNCTION (this);

    //SetCrcType (1); // 1 for CRC16
    uint16_t oldCrcValue = GetCrc16Value ();
    SetCrcValue (0); // Initialize CRC value to 0 for calculation
    std::vector <std::uint8_t> m_canonical_block_cbor_encoding = json::to_cbor(m_canonical_block); // CBOR encoding of the primary bundle block -- needed for CRC calculation
    uint32_t cborSize = m_canonical_block_cbor_encoding.size ();
    if (cborSize == 0)
    {
        return 0;
    }
    uint8_t *cborData = &m_canonical_block_cbor_encoding[0];
    uint8_t dataBuffer[cborSize];
    std::memcpy(dataBuffer, cborData, cborSize);
    //uint16_t crcValue = crcSlow(dataBuffer, cborSize);
    uint16_t crcValue = CalcCrc16Slow (dataBuffer, cborSize);
    SetCrcValue (oldCrcValue); // Restore original CRC value
    return crcValue;

    // TODO:  Implement CRC calculation
    // remove CRC value from field (if present) (replace with zeros), then encode block in CBOR
    // calculate CRC value based on CRC type from CBOR encoding
    // restore original CrcValue to field and return newly calculated CRC value
    //return 0;
}

uint16_t
BpCanonicalBlock::CalcCrc16Slow (uint8_t const message[], uint32_t nBytes)
{
    NS_LOG_FUNCTION (this);
    //const char CRC_NAME[] =		"CRC-CCITT";
    //int32_t POLYNOMIAL =		0x1021; //
    //int32_t INITIAL_REMAINDER =	0xFFFF; //
    //int32_t FINAL_XOR_VALUE =	0x0000; //
    //int32_t REFLECT_DATA =  	FALSE; //
    //int32_t REFLECT_REMAINDER =	FALSE; //
    //int32_t CHECK_VALUE =		0x29B1;

    uint16_t remainder = 0xFFFF;
    uint32_t byte = 0;
    uint8_t bit = 0;

    /*
     * Perform modulo-2 division, a byte at a time.
     */
    for (byte = 0; byte < nBytes; ++byte)
    {
        /*
         * Bring the next byte into the remainder.
         */
        remainder ^= (message[byte] << ((8 * sizeof(uint16_t)) - 8));

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & (1 << ((8 * sizeof(uint16_t)) - 1)))
            {
                remainder = (remainder << 1) ^ 0x1021;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }

    /*
     * The final remainder is the CRC result.
     */
    return (remainder ^ 0x0000);
}   

uint32_t
BpCanonicalBlock::CalcCrc32Value ()
{
    NS_LOG_FUNCTION (this);
    // TODO:  Implement CRC32 calculation
    return 0;
}

bool
BpCanonicalBlock::CheckCrcValue () // false for mismatch/calculation error; true for crc match
{
    NS_LOG_FUNCTION (this);
    if (!m_canonical_block.contains (CANONICAL_BLOCK_FIELD_CRC_VALUE))
    {
        NS_LOG_FUNCTION (this << " CRC value field not present in canonical block");
        return false;
    }
    uint8_t crcType = GetCrcType ();
    if (crcType == 0)
    {
        NS_LOG_FUNCTION (this << " No CRC value set for canonical block");
        return true;
    }
    else if (crcType == 1)
    {
        uint16_t crcValue = GetCrc16Value ();
        uint16_t newCrcValue = CalcCrc16Value ();
        if (crcValue == newCrcValue)
        {
            return true;
        }
        NS_LOG_FUNCTION (this << " CRC16 mismatch: original: " << crcValue << " != fresh: " << newCrcValue);
        return false;
    }
    else if (crcType == 2)
    {
        NS_LOG_FUNCTION (this << " CRC32 not yet implemented");
        return false;
    }
    else
    {
        NS_LOG_FUNCTION (this << " CRC type not supported");
        return false;
    }
}

bool
BpCanonicalBlock::IsEmpty () const
{
    NS_LOG_FUNCTION (this);
    if (m_canonical_block[CANONICAL_BLOCK_FIELD_TYPE_CODE] == BLOCK_TYPE_UNSPECIFIED)
    {
        return true;
    }
    return false;
}

} // namespace ns3
