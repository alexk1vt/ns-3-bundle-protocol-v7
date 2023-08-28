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
#ifndef BP_PRIMARY_BLOCK_H
#define BP_PRIMARY_BLOCK_H

#include <stdint.h>
#include <string>
#include <ctime>
#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/buffer.h"
#include "ns3/sequence-number.h"
#include "bp-endpoint-id.h"
#include "json.hpp"
using json = nlohmann::json;

namespace ns3 {

/**
 * \brief Bundle protocol primary block
 * 
 * The structure of the primary bundle block, which is defined in section 4.3.1 of RFC 9171.
 * 
*/
class BpPrimaryBlock
{
public:
  BpPrimaryBlock ();
  BpPrimaryBlock(BpEndpointId src, BpEndpointId dst, uint32_t payloadSize);
  virtual ~BpPrimaryBlock ();

  // Setters
    /**
     * \brief Set the Primary Block from JSON
     * 
     * \param jsonPrimaryBlock the Primary Block in JSON format
     */
    void SetPrimaryBlockFromJson (json jsonPrimaryBlock);

    /**
     * \brief Set the version of bundle protocol
     *
     * \param ver the version of bundle protocol
     */
    void SetVersion (uint8_t ver);

    /**
     * \brief Set Bundle Processing Control Flags
     * 
     * \param flags Bundle Processing Control Flags
     */
    void SetProcessingFlags (uint64_t flags);

    /**
     * \brief Set the destination endpoint id
     * 
     * \param dst the destination endpoint id
     */
    void SetDestinationEid (const BpEndpointId &dst);

    /**
     * \brief Set the source endpoint id
     * 
     * \param src the source endpoint id
     */
    void SetSourceEid (const BpEndpointId &src);

    /**
     * \brief Set the report endpoint id
     * 
     * \param report the report endpoint id
     */
    void SetReportEid (const BpEndpointId &report);

    /**
     * \brief Set the creation timestamp
     * 
     * \param timestamp the creation timestamp
     */
    void SetCreationTimestamp (const std::time_t &timestamp); // NOTE: Parameter is not correct - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp

    /**
     * \brief Set the lifetime of the bundle payload
     * 
     * \param lifetime the lifetime of the bundle payload
     */
    void SetLifetime (uint32_t lifetime);

    /**
     * \brief Set the Fragment offset (only if bundle is a fragment)
     * 
     * \param offset the Fragment offset
     */
    void SetFragmentOffset (uint32_t offset);

    /**
     * \brief Set the total length of the bundle payload (only if bundle is a fragment)
     * 
     * \param len the total length of the bundle payload
     */
    void SetAduLength (uint32_t len);

    /**
     * \brief Set the CRC Type
     * 
     * \param crcType the CRC Type
     */
    void SetCrcType (uint8_t crcType);

    /**
     * \brief Set the CRC of the primary block (only if CRC Type is not 0)
     * 
     * \param crc the CRC of the primary block
     */
    void SetCrcValue (uint32_t crc);

    /**
     * \brief Set the bundle to be a fragment
     * 
     * \param isFragment the bundle to be a fragment
     */
    void SetIsFragment (bool isFragment);

    /**
     * \brief Set the bundle not to be fragmented
     * 
     * \param doNotFragment the bundle not to be fragmented
     */
    void SetDoNotFragment (bool doNotFragment);

    /**
     * \brief Set bundle as an administrative record
     * 
     * \param isAdmRecord bundle as an administrative record
     */
    void SetAduIsAdminRecord (bool isAdminRecord);

    /**
     * \brief Set Application Acknowledgement Requested
     * 
     * \param ackRequest Application Acknowledgement Requested
     */
    void SetAppAckRequest (bool ackRequest);

    /**
     * \brief Set Status Time in Reports Requested
     * 
     * \param statusTimeRequest Status Time in Reports Requested
     */
    void SetStatusTimeRequest (bool statusTimeRequest);

    /**
     * \brief Set Reception Report Requested
     * 
     * \param reportRequest Reception Report Requested
     */
    void SetReceptionReportRequest (bool reportRequest);

    /**
     * \brief Set Forwarding Report Requested
     * 
     * \param reportRequest Forwarding Report Requested
     */
    void SetForwardReportRequest (bool reportRequest);

    /**
     * \brief Set Delivery Report Requested
     * 
     * \param reportRequest Delivery Report Requested
     */
    void SetDeliveryReportRequest (bool reportRequest);

    /**
     * \brief Set Deletion Report Requested
     * 
     * \param reportRequest Deletion Report Requested
     */
    void SetDeletionReportRequest (bool reportRequest);

    /**
     * \brief Rebuild the primary bundle block with current configured values
    */
    //void RebuildBlock ();

  // Getters

    /**
     * \return Is bundle a fragment?
    */
    bool IsFragment () const;

    /**
     * \return Is the bundle not to be fragmented?
     */
    bool DoNotFragment () const;

    /**
     * \return Is a delivery report requested?
     */
    bool DeliveryReportRequested () const;

    /**
     * \return The version number of the bundle protocol
     */
    uint8_t GetVersion () const;

    /**
     * \return The destination endpoint id
     */
    BpEndpointId GetDestinationEid () const;

    /**
     * \return The source endpoint id
     */
    BpEndpointId GetSourceEid () const;

    /**
     * \return The report endpoint id
     */
    BpEndpointId GetReportToEid () const;

    /**
     * \return The creation timestamp
     */
    std::time_t GetCreationTimestamp () const; // NOTE: Return type is not correct - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp

    /**
     * \return The lifetime of the bundle payload
     */
    uint32_t GetLifetime () const;

    /**
     * \return The Fragment offset (only if bundle is a fragment)
     */
    uint32_t GetFragmentOffset () const;

    /**
     * \return The total length of the bundle payload (only if bundle is a fragment)
     */
    uint32_t GetAduLength () const;

    /**
     * \return The CRC Type
     */
    uint8_t GetCrcType () const;

    /**
     * \return The CRC of the primary block (only if CRC Type is not 0)
     */
    uint32_t GetCrc32Value () const;
    uint16_t GetCrc16Value () const;

    int GenerateCrcValue ();
    uint16_t CalcCrc16Value (); // overwrites existing CRC value
    uint32_t CalcCrc32Value (); // overwrites existing CRC value
    bool CheckCrcValue (); // false for mismatch/calculation error; true for crc match
    uint16_t CalcCrc16Slow (uint8_t const message[], uint32_t nBytes);

    /**
     * \return The primary bundle block JSON
     */
    json GetJson () const;

    /**
     * \brief Dump the primary bundle block to NS_LOG_FUNCTION
     * 
     * \return None
    */
    void dump () const;

    // TODO:  Add the rest of the getters

  /*
  * Bundle processing control flags
  */
    //typedef enum {
    enum class BundleProcessingFlags : uint64_t {
        BUNDLE_IS_FRAGMENT             = 1 << 0,
        BUNDLE_ADU_IS_ADMIN_RECORD     = 1 << 1,
        BUNDLE_DO_NOT_FRAGMENT         = 1 << 2,
        BUNDLE_APP_ACK_REQUEST         = 1 << 5,
        BUNDLE_STATUS_TIME_REQUEST     = 1 << 6,
        BUNDLE_RECEPTION_REPORT_REQUEST= 1 << 14,
        BUNDLE_FORWARD_REPORT_REQUEST  = 1 << 16,
        BUNDLE_DELIVERY_REPORT_REQUEST = 1 << 17,
        BUNDLE_DELETION_REPORT_REQUEST = 1 << 18,
    };// BundleProcessingFlags;

  /*
  * Primary Block Field Index values
  */
  //typedef enum {
  //  PRIMARY_BLOCK_VERSION = 0,
  //  PRIMARY_BLOCK_BUNDLE_PROCESSING_FLAGS = 1,
  //  PRIMARY_BLOCK_CRC_TYPE = 2,
  //  PRIMARY_BLOCK_DESTINATION = 3,
  //  PRIMARY_BLOCK_SOURCE = 4,
  //  PRIMARY_BLOCK_REPORT_TO = 5,
  //  PRIMARY_BLOCK_CREATION_TIMESTAMP = 6,
  //  PRIMARY_BLOCK_LIFETIME = 7,
  //  PRIMARY_BLOCK_FRAGMENT_OFFSET = 8,
  //  PRIMARY_BLOCK_ADU_LENGTH = 9,
  //  PRIMARY_BLOCK_CRC_VALUE = 10
  //} PrimaryBlockFields;

  std::string PRIMARY_BLOCK_FIELD_VERSION = "00";
  std::string PRIMARY_BLOCK_FIELD_BUNDLE_PROCESSING_FLAGS = "01";
  std::string PRIMARY_BLOCK_FIELD_CRC_TYPE = "02";
  std::string PRIMARY_BLOCK_FIELD_DESTINATION = "03";
  std::string PRIMARY_BLOCK_FIELD_SOURCE = "04";
  std::string PRIMARY_BLOCK_FIELD_REPORT_TO = "05";
  std::string PRIMARY_BLOCK_FIELD_CREATION_TIMESTAMP = "06";
  std::string PRIMARY_BLOCK_FIELD_LIFETIME = "07";
  std::string PRIMARY_BLOCK_FIELD_FRAGMENT_OFFSET = "08";
  std::string PRIMARY_BLOCK_FIELD_ADU_LENGTH = "09";
  std::string PRIMARY_BLOCK_FIELD_CRC_VALUE = "10";

private:
    //uint8_t m_version;                      // the version of bundle protocol "version"
    //uint32_t m_blockNumber;                 // block number "block_number"
    //uint64_t m_processingFlags;             // bundle processing control flags "processing_flags"
    BpEndpointId m_destEndpointId;          // destination endpoint id "destination"
    BpEndpointId m_sourceEndpointId;        // source endpoint id "source"
    BpEndpointId m_reportToEndPointId;      // report endpoint id "report_to"
    //std::time_t m_creationTimestamp;        // creation time "creation_timestamp" NOTE: Parameter is not correct - TODO: reference Section 4.2.7 of RFC 9171 for proper definition of timestamp
    //uint32_t m_lifetime;                    // lifetime in seconds "lifetime"
    //uint32_t m_fragmentOffset;              // fragementation offset "fragment_offset"
    //uint32_t m_aduLength;                   // application data unit length "adu_length"
    //uint8_t m_crcType;                      // CRC Type "crc_type"
    //uint32_t m_crcValue;                    // CRC of primary block "crc_value"

    // primary bundle block, section 4.3.1, RFC 9171
    json m_primary_bundle_block;            // primary bundle block

};

} // namespace ns3

#endif /* BP_PRIMARY_BLOCK_H */
