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
#include <stdio.h>
#include <string>

#include <vector>
#include <ctime>

#define RFC_DATE_2000 946684800

NS_LOG_COMPONENT_DEFINE ("BpPrimaryBlock");

namespace ns3 {

/* Public */
BpPrimaryBlock::BpPrimaryBlock ()
  : m_version (0x7),
    m_processingFlags (0),
    m_creationTimestamp (0),
    m_lifetime (0),
    m_fragmentOffset (0),
    m_aduLength (0),
    m_crcType (0),
    m_crcValue (0)
{
    NS_LOG_FUNCTION (this);

    m_destEndpointId = BpEndpointId ();
    m_sourceEndpointId = BpEndpointId ();
    m_reportToEndPointId = BpEndpointId ();

    // set creation timestamp to current time
    m_creationTimestamp = std::time (NULL) - RFC_DATE_2000;
    // from bp-header.cc:  m_createTimestamp = timestamp - RFC_DATE_2000;

    // build the json primary block - ordering is specified in RFC 9171 and remains consistent with the order of the primary block fields
    m_primary_bundle_block["version"] = m_version;
    m_primary_bundle_block["processing_flags"] = m_processingFlags;
    m_primary_bundle_block["crc_type"] = m_crcType;
    m_primary_bundle_block["destination"] = m_destEndpointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["source"] = m_sourceEndpointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["report_to"] = m_reportToEndPointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["creation_timestamp"] = m_creationTimestamp;
    m_primary_bundle_block["lifetime"] = m_lifetime;
    if (m_processingFlags & BUNDLE_IS_FRAGMENT)
    {
        m_primary_bundle_block["fragment_offset"] = m_fragmentOffset;
    }
    m_primary_bundle_block["adu_length"] = m_aduLength;
    //if (m_crcType != 0)
    // TODO:  calculate CRC value
    //   m_primary_bundle_block["crc_value"] = m_crcValue;
}

BpPrimaryBlock::BpPrimaryBlock(BpEndpointId src, BpEndpointId dst, uint32_t payloadSize)
    : m_version (0x7),
    m_processingFlags (0),
    m_creationTimestamp (0),
    m_lifetime (0),
    m_fragmentOffset (0),
    m_aduLength (payloadSize),
    m_crcType (0),
    m_crcValue (0)
{
    NS_LOG_FUNCTION (this);
    m_sourceEndpointId = src;
    m_destEndpointId = dst;

    // set creation timestamp to current time
    m_creationTimestamp = std::time (NULL) - RFC_DATE_2000;

    // build the json primary block - ordering is specified in RFC 9171 and remains consistent with the order of the primary block fields
    m_primary_bundle_block["version"] = m_version;
    m_primary_bundle_block["processing_flags"] = m_processingFlags;
    m_primary_bundle_block["crc_type"] = m_crcType;
    m_primary_bundle_block["destination"] = m_destEndpointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["source"] = m_sourceEndpointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["report_to"] = m_reportToEndPointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["creation_timestamp"] = m_creationTimestamp;
    m_primary_bundle_block["lifetime"] = m_lifetime;
    if (m_processingFlags & BUNDLE_IS_FRAGMENT)
    {
        m_primary_bundle_block["fragment_offset"] = m_fragmentOffset;
    }
    m_primary_bundle_block["adu_length"] = m_aduLength;
}

BpPrimaryBlock::~BpPrimaryBlock ()
{
    NS_LOG_FUNCTION (this);
}

void
BpPrimaryBlock::SetPrimaryBlockFromJson (json jsonPrimaryBlock)
{
    NS_LOG_FUNCTION (this << jsonPrimaryBlock.dump ());
    m_primary_bundle_block = jsonPrimaryBlock;
    m_version = m_primary_bundle_block["version"];
    m_processingFlags = m_primary_bundle_block["processing_flags"];
    m_crcType = m_primary_bundle_block["crc_type"];
    BpEndpointId tempEid(m_primary_bundle_block["destination"]);
    m_destEndpointId = tempEid;
    tempEid = BpEndpointId(m_primary_bundle_block["source"]);
    m_sourceEndpointId = tempEid;
    if (m_primary_bundle_block.contains("report_to"))
    {
        tempEid = BpEndpointId(m_primary_bundle_block["report_to"]);
        m_reportToEndPointId = tempEid;
    }
    m_creationTimestamp = m_primary_bundle_block["creation_timestamp"];
    m_lifetime = m_primary_bundle_block["lifetime"];
    if (m_processingFlags & BUNDLE_IS_FRAGMENT)
    {
        m_fragmentOffset = m_primary_bundle_block["fragment_offset"];
    }
    m_aduLength = m_primary_bundle_block["adu_length"];
    //if (m_crcType != 0) // TODO:  Implement this!
}

void
BpPrimaryBlock::SetVersion (uint8_t ver)
{
    NS_LOG_FUNCTION (this << (int) ver);
    m_version = ver;
}

void
BpPrimaryBlock::SetProcessingFlags (uint64_t flags) // NOTE: not properly implemented - TODO: reference RFC 9171
{
    NS_LOG_FUNCTION (this << (int) flags);
    m_processingFlags = flags;
}

void
BpPrimaryBlock::SetDestinationEid (const BpEndpointId &eid)
{
    NS_LOG_FUNCTION (this << eid.Uri ());
    m_destEndpointId = eid;
}

void
BpPrimaryBlock::SetSourceEid (const BpEndpointId &eid)
{
    NS_LOG_FUNCTION (this << eid.Uri ());
    m_sourceEndpointId = eid;
}

void
BpPrimaryBlock::SetReportEid (const BpEndpointId &eid)
{
    NS_LOG_FUNCTION (this << eid.Uri ());
    m_reportToEndPointId = eid;
}

void
BpPrimaryBlock::SetCreationTimestamp (const std::time_t &timestamp) // // NOTE: Parameter is not correct - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp
{
    NS_LOG_FUNCTION (this << timestamp);
    m_creationTimestamp = timestamp;
}

void
BpPrimaryBlock::SetLifetime (uint32_t lifetime)
{
    NS_LOG_FUNCTION (this << lifetime);
    m_lifetime = lifetime;
}

void
BpPrimaryBlock::SetFragmentOffset (uint32_t offset)
{
    NS_LOG_FUNCTION (this << offset);
    m_fragmentOffset = offset;
}

void
BpPrimaryBlock::SetAduLength (uint32_t length)
{
    NS_LOG_FUNCTION (this << length);
    m_aduLength = length;
}

void
BpPrimaryBlock::SetCrcType (uint8_t crcType)
{
    NS_LOG_FUNCTION (this << (int) crcType);
    m_crcType = crcType;
}

void
BpPrimaryBlock::SetCrcValue (uint32_t crcValue)
{
    NS_LOG_FUNCTION (this << crcValue);
    m_crcValue = crcValue;
}

void
BpPrimaryBlock::SetIsFragment (bool isFragment)
{
    NS_LOG_FUNCTION (this << isFragment);
    if (isFragment)
    {
        m_processingFlags |= BUNDLE_IS_FRAGMENT;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_IS_FRAGMENT;
    }
}

void
BpPrimaryBlock::SetAduIsAdminRecord (bool isAdminRecord)
{
    NS_LOG_FUNCTION (this << isAdminRecord);
    if (isAdminRecord)
    {
        m_processingFlags |= BUNDLE_ADU_IS_ADMIN_RECORD;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_ADU_IS_ADMIN_RECORD;
    }
}

void
BpPrimaryBlock::SetDoNotFragment (bool doNotFragment)
{
    NS_LOG_FUNCTION (this << doNotFragment);
    if (doNotFragment)
    {
        m_processingFlags |= BUNDLE_DO_NOT_FRAGMENT;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_DO_NOT_FRAGMENT;
    }
}

void
BpPrimaryBlock::SetAppAckRequest (bool ackRequest)
{
    NS_LOG_FUNCTION (this << ackRequest);
    if (ackRequest)
    {
        m_processingFlags |= BUNDLE_APP_ACK_REQUEST;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_APP_ACK_REQUEST;
    }
}

void
BpPrimaryBlock::SetStatusTimeRequest (bool statusTimeRequest)
{
    NS_LOG_FUNCTION (this << statusTimeRequest);
    if (statusTimeRequest)
    {
        m_processingFlags |= BUNDLE_STATUS_TIME_REQUEST;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_STATUS_TIME_REQUEST;
    }
}

void
BpPrimaryBlock::SetReceptionReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_processingFlags |= BUNDLE_RECEPTION_REPORT_REQUEST;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_RECEPTION_REPORT_REQUEST;
    }
}

void
BpPrimaryBlock::SetForwardReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_processingFlags |= BUNDLE_FORWARD_REPORT_REQUEST;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_FORWARD_REPORT_REQUEST;
    }
}

void
BpPrimaryBlock::SetDeliveryReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_processingFlags |= BUNDLE_DELIVERY_REPORT_REQUEST;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_DELIVERY_REPORT_REQUEST;
    }
}

void
BpPrimaryBlock::SetDeletionReportRequest (bool reportRequest)
{
    NS_LOG_FUNCTION (this << reportRequest);
    if (reportRequest)
    {
        m_processingFlags |= BUNDLE_DELETION_REPORT_REQUEST;
    }
    else
    {
        m_processingFlags &= ~BUNDLE_DELETION_REPORT_REQUEST;
    }
}

void
BpPrimaryBlock::RebuildBlock ()
{
    NS_LOG_FUNCTION (this);
    // rebuild the json primary block - ordering is specified in RFC 9171 and remains consistent with the order of the primary block fields
    m_primary_bundle_block["version"] = m_version;
    m_primary_bundle_block["processing_flags"] = m_processingFlags;
    m_primary_bundle_block["crc_type"] = m_crcType;
    m_primary_bundle_block["destination"] = m_destEndpointId.Uri (); //GetEidString ();
    m_primary_bundle_block["source"] = m_sourceEndpointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["report_to"] = m_reportToEndPointId.Uri (); //.GetEidString ();
    m_primary_bundle_block["creation_timestamp"] = m_creationTimestamp;
    m_primary_bundle_block["lifetime"] = m_lifetime;
    if (m_processingFlags & BUNDLE_IS_FRAGMENT)
    {
        m_primary_bundle_block["fragment_offset"] = m_fragmentOffset;
    }
    m_primary_bundle_block["adu_length"] = m_aduLength;
    //if (m_crcType != 0)
    // TODO:  implement this
}

bool
BpPrimaryBlock::IsFragment () const
{
    NS_LOG_FUNCTION (this);
    return (m_processingFlags & BUNDLE_IS_FRAGMENT);  // NOTE: not properly implemented - TODO: reference RFC 9171
}

bool
BpPrimaryBlock::DoNotFragment () const
{
    NS_LOG_FUNCTION (this);
    return (m_processingFlags & BUNDLE_DO_NOT_FRAGMENT);  // NOTE: not properly implemented - TODO: reference RFC 9171
}

bool
BpPrimaryBlock::DeliveryReportRequested () const
{
    NS_LOG_FUNCTION (this);
    return (m_processingFlags & BUNDLE_DELIVERY_REPORT_REQUEST);  // NOTE: not properly implemented - TODO: reference RFC 9171
}

uint8_t
BpPrimaryBlock::GetVersion () const
{
    NS_LOG_FUNCTION (this);
    return m_version;
}

BpEndpointId
BpPrimaryBlock::GetDestinationEid () const
{
    NS_LOG_FUNCTION (this);
    return m_destEndpointId;
}

BpEndpointId
BpPrimaryBlock::GetSourceEid () const
{
    NS_LOG_FUNCTION (this);
    return m_sourceEndpointId;
}

BpEndpointId
BpPrimaryBlock::GetReportToEid () const
{
    NS_LOG_FUNCTION (this);
    return m_reportToEndPointId;
}

std::time_t
BpPrimaryBlock::GetCreationTimestamp () const // NOTE: not properly implemented - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp
{
    NS_LOG_FUNCTION (this);
    return m_creationTimestamp;
}

uint32_t
BpPrimaryBlock::GetLifetime () const
{
    NS_LOG_FUNCTION (this);
    return m_lifetime;
}

uint32_t
BpPrimaryBlock::GetFragmentOffset () const
{
    NS_LOG_FUNCTION (this);
    return m_fragmentOffset;
}

uint32_t
BpPrimaryBlock::GetAduLength () const
{
    NS_LOG_FUNCTION (this);
    return m_aduLength;
}

uint8_t
BpPrimaryBlock::GetCrcType () const
{
    NS_LOG_FUNCTION (this);
    return m_crcType;
}

uint32_t
BpPrimaryBlock::GetCrcValue () const
{
    NS_LOG_FUNCTION (this);
    return m_crcValue;
    // can test whether CRC value was previously calculated by checking if this returns 0
}

json
BpPrimaryBlock::GetJson () const
{
    NS_LOG_FUNCTION (this);
    return m_primary_bundle_block;
}

uint32_t
BpPrimaryBlock::CalcCrcValue () const
{
    NS_LOG_FUNCTION (this);
    std::vector <std::uint8_t> m_primary_block_cbor_encoding = json::to_cbor(m_primary_bundle_block); // CBOR encoding of the primary bundle block -- needed for CRC calculation
    // TODO:  Implement CRC calculation
    // remove CRC value from field (if present) (replace with zeros), then encode block in CBOR
    // calculate CRC value based on CRC type from CBOR encoding
    // restore original CrcValue to field and return newly calculated CRC value
    return 0;
}
    
} // namespace ns3
