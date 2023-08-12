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
#include "bp-primary-block.h"
//#include "bp-crc.h"
#include <stdio.h>
#include <string>

#include <vector>
#include <ctime>

#define RFC_DATE_2000 946684800

NS_LOG_COMPONENT_DEFINE ("BpPrimaryBlock");

namespace ns3 {

/* Public */
BpPrimaryBlock::BpPrimaryBlock ()
{
    NS_LOG_FUNCTION (this);

    m_destEndpointId = BpEndpointId ();
    m_sourceEndpointId = BpEndpointId ();
    m_reportToEndPointId = BpEndpointId ();

    // set creation timestamp to current time

    // build the json primary block - ordering is specified in RFC 9171 and remains consistent with the order of the primary block fields
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_VERSION] = 0x7;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = static_cast<uint64_t>(0);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_TYPE] = 0;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION] = ""; 
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE] = ""; 
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO] = ""; 
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CREATION_TIMESTAMP] = std::time (NULL) - RFC_DATE_2000;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_LIFETIME] = 0;

    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_ADU_LENGTH] = 0;
    //if (m_crcType != 0)
    // TODO:  calculate CRC value
    //   m_primary_bundle_block["crc_value"] = m_crcValue;
}

BpPrimaryBlock::BpPrimaryBlock(BpEndpointId src, BpEndpointId dst, uint32_t payloadSize)

{
    NS_LOG_FUNCTION (this);
    m_sourceEndpointId = src;
    m_destEndpointId = dst;

    // set creation timestamp to current time
    //m_creationTimestamp = std::time (NULL) - RFC_DATE_2000;

    // build the json primary block - ordering is specified in RFC 9171 and remains consistent with the order of the primary block fields
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_VERSION] = 0x7;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = 0;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_TYPE] = 0;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION] = dst.Uri (); //.GetEidString ();
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE] = src.Uri (); //.GetEidString ();
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO] = ""; //.GetEidString ();
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CREATION_TIMESTAMP] = std::time (NULL) - RFC_DATE_2000;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_LIFETIME] = 0;
    //if (m_processingFlags & BUNDLE_IS_FRAGMENT)
    //{
    //    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_FRAGMENT_OFFSET] = m_fragmentOffset;
    //}
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_ADU_LENGTH] = payloadSize;
}

BpPrimaryBlock::~BpPrimaryBlock ()
{
    NS_LOG_FUNCTION (this);
}

void
BpPrimaryBlock::SetPrimaryBlockFromJson (json jsonPrimaryBlock)
{
    NS_LOG_FUNCTION (this);
    m_primary_bundle_block = jsonPrimaryBlock;
    if (!m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE].get <std::string> ().empty ())
    {
        BpEndpointId tempEid(m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE]);
        m_sourceEndpointId = tempEid;
    }
    if (!m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION].get <std::string> ().empty ())
    {
        BpEndpointId tempEid(m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION]);
        m_destEndpointId = tempEid;
    }
    if (!m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO].get <std::string> ().empty ())
    {
        BpEndpointId tempEid(m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO]);
        m_reportToEndPointId = tempEid;
    }
}

void
BpPrimaryBlock::SetVersion (uint8_t ver)
{
    NS_LOG_FUNCTION (this << (int) ver);
    //m_version = ver;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_VERSION] = ver;
}

void
BpPrimaryBlock::SetProcessingFlags (uint64_t flags) // NOTE: not properly implemented - TODO: reference RFC 9171
{
    NS_LOG_FUNCTION (this << (int) flags);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = flags;
}

void
BpPrimaryBlock::SetDestinationEid (const BpEndpointId &eid)
{
    NS_LOG_FUNCTION (this << eid.Uri ());
    m_destEndpointId = eid;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION] = m_destEndpointId.Uri (); 
}

void
BpPrimaryBlock::SetSourceEid (const BpEndpointId &eid)
{
    NS_LOG_FUNCTION (this << eid.Uri ());
    m_sourceEndpointId = eid;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE] = m_sourceEndpointId.Uri (); 
}

void
BpPrimaryBlock::SetReportEid (const BpEndpointId &eid)
{
    NS_LOG_FUNCTION (this << eid.Uri ());
    m_reportToEndPointId = eid;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO] = m_reportToEndPointId.Uri ();
}

void
BpPrimaryBlock::SetCreationTimestamp (const std::time_t &timestamp) // // NOTE: Parameter is not correct - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp
{
    NS_LOG_FUNCTION (this << timestamp);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CREATION_TIMESTAMP] = timestamp;
}

void
BpPrimaryBlock::SetLifetime (uint32_t lifetime)
{
    NS_LOG_FUNCTION (this << lifetime);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_LIFETIME] = lifetime;
}

void
BpPrimaryBlock::SetFragmentOffset (uint32_t offset)
{
    NS_LOG_FUNCTION (this << offset);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_FRAGMENT_OFFSET] = offset;
}

void
BpPrimaryBlock::SetAduLength (uint32_t length)
{
    NS_LOG_FUNCTION (this << length);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_ADU_LENGTH] = length;
}

void
BpPrimaryBlock::SetCrcType (uint8_t crcType)
{
    NS_LOG_FUNCTION (this << (int) crcType);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_TYPE] = crcType;
}

void
BpPrimaryBlock::SetCrcValue (uint32_t crcValue)
{
    NS_LOG_FUNCTION (this << crcValue);
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_VALUE] = crcValue;
}

void
BpPrimaryBlock::SetIsFragment (bool isFragment)
{
    NS_LOG_FUNCTION (this << isFragment);
    if (isFragment)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_IS_FRAGMENT);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_IS_FRAGMENT);
    }
}

void
BpPrimaryBlock::SetAduIsAdminRecord (bool isAdminRecord)
{
    NS_LOG_FUNCTION (this << isAdminRecord);
    if (isAdminRecord)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_ADU_IS_ADMIN_RECORD);
    }
    else
    {
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_ADU_IS_ADMIN_RECORD);
    }
}

void
BpPrimaryBlock::SetDoNotFragment (bool doNotFragment)
{
    NS_LOG_FUNCTION (this << doNotFragment);
    if (doNotFragment)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DO_NOT_FRAGMENT);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DO_NOT_FRAGMENT);
    }
}

void
BpPrimaryBlock::SetAppAckRequest (bool ackRequest)
{
    NS_LOG_FUNCTION (this << ackRequest);
    if (ackRequest)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_APP_ACK_REQUEST);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_APP_ACK_REQUEST);
    }
}

void
BpPrimaryBlock::SetStatusTimeRequest (bool statusTimeRequest)
{
    NS_LOG_FUNCTION (this << statusTimeRequest);
    if (statusTimeRequest)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_STATUS_TIME_REQUEST);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_STATUS_TIME_REQUEST);
    }
}

void
BpPrimaryBlock::SetReceptionReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_RECEPTION_REPORT_REQUEST);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_RECEPTION_REPORT_REQUEST);
    }
}

void
BpPrimaryBlock::SetForwardReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_FORWARD_REPORT_REQUEST);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_FORWARD_REPORT_REQUEST);
    }
}

void
BpPrimaryBlock::SetDeliveryReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DELIVERY_REPORT_REQUEST);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DELIVERY_REPORT_REQUEST);
    }
}

void
BpPrimaryBlock::SetDeletionReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () | static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DELETION_REPORT_REQUEST);
    }
    else
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & ~static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DELETION_REPORT_REQUEST);
    }
}
/*
void
BpPrimaryBlock::RebuildBlock ()
{
    NS_LOG_FUNCTION (this);
    // rebuild the json primary block - ordering is specified in RFC 9171 and remains consistent with the order of the primary block fields
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_VERSION] = m_version;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS] = m_processingFlags;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_TYPE] = m_crcType;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION] = m_destEndpointId.Uri (); //GetEidString ();
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE] = m_sourceEndpointId.Uri (); //.GetEidString ();
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO] = m_reportToEndPointId.Uri (); //.GetEidString ();
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CREATION_TIMESTAMP] = m_creationTimestamp;
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_LIFETIME] = m_lifetime;
    if (m_processingFlags & BUNDLE_IS_FRAGMENT)
    {
        m_primary_bundle_block[PRIMARY_BLOCK_FIELD_FRAGMENT_OFFSET] = m_fragmentOffset;
    }
    m_primary_bundle_block[PRIMARY_BLOCK_FIELD_ADU_LENGTH] = m_aduLength;
    //if (m_crcType != 0)
    // TODO:  implement this
}
*/
bool
BpPrimaryBlock::IsFragment () const
{
    NS_LOG_FUNCTION (this);
    bool retVal = (m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_IS_FRAGMENT));  // NOTE: not properly implemented - TODO: reference RFC 9171
    NS_LOG_FUNCTION (this << " Returning: " << retVal << "Bundle processing flags: " << m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS]);
    return retVal;
}

bool
BpPrimaryBlock::DoNotFragment () const
{
    NS_LOG_FUNCTION (this);
    return (m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DO_NOT_FRAGMENT));  // NOTE: not properly implemented - TODO: reference RFC 9171
}

bool
BpPrimaryBlock::DeliveryReportRequested () const
{
    NS_LOG_FUNCTION (this);
    return (m_primary_bundle_block[PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS].get<uint64_t> () & static_cast<uint64_t>(BundleProcessingFlags::BUNDLE_DELIVERY_REPORT_REQUEST));  // NOTE: not properly implemented - TODO: reference RFC 9171
}

uint8_t
BpPrimaryBlock::GetVersion () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_VERSION];
}

BpEndpointId
BpPrimaryBlock::GetDestinationEid () const
{
    NS_LOG_FUNCTION (this);
    BpEndpointId tempEid(m_primary_bundle_block[PRIMARY_BLOCK_FIELD_DESTINATION]);
    return tempEid;
}

BpEndpointId
BpPrimaryBlock::GetSourceEid () const
{
    NS_LOG_FUNCTION (this);
    BpEndpointId tempEid(m_primary_bundle_block[PRIMARY_BLOCK_FIELD_SOURCE]);
    return tempEid;
}

BpEndpointId
BpPrimaryBlock::GetReportToEid () const
{
    NS_LOG_FUNCTION (this);
    BpEndpointId tempEid(m_primary_bundle_block[PRIMARY_BLOCK_FIELD_REPORT_TO]);
    return tempEid;
}

std::time_t
BpPrimaryBlock::GetCreationTimestamp () const // NOTE: not properly implemented - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CREATION_TIMESTAMP];
}

uint32_t
BpPrimaryBlock::GetLifetime () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_LIFETIME];
}

uint32_t
BpPrimaryBlock::GetFragmentOffset () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_FRAGMENT_OFFSET];
}

uint32_t
BpPrimaryBlock::GetAduLength () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_ADU_LENGTH];
}

uint8_t
BpPrimaryBlock::GetCrcType () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_TYPE];
}

uint32_t
BpPrimaryBlock::GetCrc32Value () const
{
    NS_LOG_FUNCTION (this);
    if (!m_primary_bundle_block.contains(PRIMARY_BLOCK_FIELD_CRC_VALUE))
    {
        NS_LOG_FUNCTION (this << " CRC value field not present in primary block");
        return 0;
    }
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_VALUE];
    // can test whether CRC value was previously calculated by checking if this returns 0
}

uint16_t
BpPrimaryBlock::GetCrc16Value () const
{
    NS_LOG_FUNCTION (this);
    if (!m_primary_bundle_block.contains(PRIMARY_BLOCK_FIELD_CRC_VALUE))
    {
        NS_LOG_FUNCTION (this << " CRC value field not present in primary block");
        return 0;
    }
    return m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_VALUE];
    // can test whether CRC value was previously calculated by checking if this returns 0
}

json
BpPrimaryBlock::GetJson () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block;
}

int
BpPrimaryBlock::GenerateCrcValue ()
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
BpPrimaryBlock::CalcCrc16Value (void)
{
    NS_LOG_FUNCTION (this);

    //SetCrcType (1); // 1 for CRC16
    uint16_t oldCrcValue = GetCrc16Value ();
    SetCrcValue (0); // Initialize CRC value to 0 for calculation
    std::vector <std::uint8_t> m_primary_block_cbor_encoding = json::to_cbor(m_primary_bundle_block); // CBOR encoding of the primary bundle block -- needed for CRC calculation
    uint32_t cborSize = m_primary_block_cbor_encoding.size ();
    if (cborSize == 0)
    {
        return 0;
    }
    uint8_t *cborData = &m_primary_block_cbor_encoding[0];
    uint8_t dataBuffer[cborSize];
    std::memcpy(dataBuffer, cborData, cborSize);
    uint16_t crcValue = CalcCrc16Slow(dataBuffer, cborSize);

    SetCrcValue (oldCrcValue); // restore original CrcValue to field
    return crcValue;

    // TODO:  Implement CRC calculation
    // remove CRC value from field (if present) (replace with zeros), then encode block in CBOR
    // calculate CRC value based on CRC type from CBOR encoding
    // restore original CrcValue to field and return newly calculated CRC value
    //return 0;
}

uint16_t
BpPrimaryBlock::CalcCrc16Slow (uint8_t const message[], uint32_t nBytes)
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
BpPrimaryBlock::CalcCrc32Value ()
{
    NS_LOG_FUNCTION (this);
    // TODO:  Implement CRC32 calculation
    return 0;
}

bool
BpPrimaryBlock::CheckCrcValue () // false for mismatch/calculation error; true for crc match
{
    NS_LOG_FUNCTION (this);
    if (!m_primary_bundle_block.contains (PRIMARY_BLOCK_FIELD_CRC_VALUE))
    {
        NS_LOG_FUNCTION (this << " CRC value field not present in primary block");
        return false;
    }
    uint8_t crcType = m_primary_bundle_block[PRIMARY_BLOCK_FIELD_CRC_TYPE];
    if (crcType == 0)
    {
        NS_LOG_FUNCTION (this << " No CRC value set for primary block");
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
    
} // namespace ns3
