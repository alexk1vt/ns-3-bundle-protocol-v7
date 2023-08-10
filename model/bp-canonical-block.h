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
#ifndef BP_CANONICAL_BLOCK_H
#define BP_CANONICAL_BLOCK_H

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
 * \brief Bundle protocol canonical block
 * 
 * The structure of canonical blocks, which is defined in section 4.3.1 of RFC 9171.
 * 
*/

// Canonical Block Field name strings for JSON
extern std::string CANONICAL_BLOCK_FIELD_TYPE_CODE; // = "00";
extern std::string CANONICAL_BLOCK_FIELD_BLOCK_NUMBER; // = "01";
extern std::string CANONICAL_BLOCK_FIELD_BLOCK_PROCESSING_FLAGS; // = "02";
extern std::string CANONICAL_BLOCK_FIELD_CRC_TYPE; // = "03";
extern std::string CANONICAL_BLOCK_FIELD_BLOCK_DATA; // = "04";
extern std::string CANONICAL_BLOCK_FIELD_CRC_VALUE; // = "05";


class BpCanonicalBlock
{
public:
  BpCanonicalBlock ();
  BpCanonicalBlock (uint8_t code, uint32_t number, uint64_t flags, uint8_t crcType, std::string data);
  BpCanonicalBlock (uint8_t code, uint8_t number, uint64_t flags, uint8_t crcType, uint32_t payloadSize, const uint8_t *payloadData);
  BpCanonicalBlock (uint8_t blockTypeCode, uint32_t payloadSize, const uint8_t *payloadData);
  virtual ~BpCanonicalBlock ();

  // Setters
    /**
     * \brief Set the canonical block from json
     * 
     * \param jsonCanonicalBlock the canonical block in json
     */
    void SetCanonicalBlockFromJson (json jsonCanonicalBlock);

    /**
     * \brief Set the block type code of the canonical block
     *
     * \param code the block type code of the canonical block
     */
    void SetBlockTypeCode (uint8_t code);

    /**
     * \brief Set the block number of the canonical block
     * 
     * \param num the block number of the canonical block
     */
    void SetBlockNumber (uint32_t num);

    /**
     * \brief Set the block processing control flags of the canonical block
     * 
     * \param flags the block processing control flags of the canonical block
     */
    void SetBlockProcessingFlags (uint64_t flags);

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
    void SetCrc (uint32_t crc);

    /**
     * \brief Set the block data
     * 
     * \param data the block data
     */
    void SetBlockData (std::string data);

    /**
     * \brief Add data to block data
     * 
     * \param data the data to add
    */
    void AddBlockData (std::string data);

    /**
     * \brief Rebuild the canonical block with current configured values
    */
    //void RebuildBlock ();

  // Getters
    /**
     * \return The block type code of the canonical block
     */
    uint8_t GetBlockTypeCode () const;

    /**
     * \return The block number of the canonical block
     */
    uint32_t GetBlockNumber () const;

    /**
     * \return The block processing control flags of the canonical block
     */
    uint64_t GetBlockProcessingFlags () const;

    /**
     * \return The CRC Type
     */
    uint8_t GetCrcType () const;
    
    /**
     * \return The CRC of the primary block (only if CRC Type is not 0)
     */
    uint32_t GetCrcValue () const;

    /**
     * \return The block data
     */
    std::string GetBlockData () const;

    /**
     * \return The block data size in bytes
    */
    uint32_t GetBlockDataSize () const;

    /**
     * \return The canonical block JSON
     */
    json GetJson () const;

    /**
     * \return The calculated CRC value of this block
    */
    uint32_t CalcCrcValue () const;

    bool IsEmpty () const;

    // TODO:  Add the rest of the getters

  /*
  * Block Type Codes
  */
    typedef enum {
        BLOCK_TYPE_PRIMARY = 0, // is 'reserved' per RFC 9171, but using to ensure primary block is sequenced first
        BLOCK_TYPE_PAYLOAD = 1,
        BLOCK_TYPE_PREVIOUS_NODE = 6,
        BLOCK_TYPE_BUNDLE_AGE = 7,
        BLOCK_TYPE_HOP_COUNT = 10,
        BLOCK_TYPE_UNSPECIFIED = 11
    } CanonicalBlockTypeCodes;

  /*
  * Block processing control flags
  */
    typedef enum {
        BLOCK_REPLICATED_EVERY_FRAGMENT      = 1 << 0,
        BLOCK_PROCESS_FAIL_REPORT_REQUEST    = 1 << 1,
        BLOCK_DELETE_BUNDLE_IF_NOT_PROCESSED = 1 << 2,
        BLOCK_DISCARD_BLOCK_IF_NOT_PROCESSED = 1 << 4
    } BlockProcessingFlags;

  /*
  * Canonical Block Field Index values
  */
  //typedef enum {
  //  CANONICAL_BLOCK_TYPE_CODE = 0,
  //  CANONICAL_BLOCK_BLOCK_NUMBER = 1,
  //  CANONICAL_BLOCK_BLOCK_PROCESSING_FLAGS = 2,
  //  CANONICAL_BLOCK_CRC_TYPE = 3,
  //  CANONICAL_BLOCK_BLOCK_DATA = 4,
  //  CANONICAL_BLOCK_CRC_VALUE = 5
  //} CanonicalBlockFields;

  

private:
    //uint8_t m_blockTypeCode;                // block type code "block_type_code"
    //uint32_t m_blockNumber;                 // block number "block_number"
    //uint64_t m_blockProcessingFlags;        // block processing control flags "block_processing_flags"
    //uint8_t m_crcType;                      // CRC Type "crc_type"
    //uint32_t m_crcValue;                    // CRC of primary block "crc_value"
    //std::string m_blockData;                     // block data "block_data"

    // the canonical block, section 4.3.2, RFC 9171
    json m_canonical_block;                 // canonical block
};

} // namespace ns3

#endif /* BP_CANONICAL_BLOCK_H */
