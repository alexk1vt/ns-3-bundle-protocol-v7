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
class BpCanonicalBlock
{
public:
  BpCanonicalBlock ();
  virtual ~BpCanonicalBlock ();

  // Setters
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
     * \brief Rebuild the canonical block with current configured values
    */
    void RebuildCanonicalBlock ();

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

    // TODO:  Add the rest of the getters

  /*
  * Block Type Codes
  */
    typedef enum {
        BLOCK_TYPE_PAYLOAD = 1,
        BLOCK_TYPE_PREVIOUS_NODE = 6,
        BLOCK_TYPE_BUNDLE_AGE = 7,
        BLOCK_TYPE_HOP_COUNT = 10
    } BlockTypeCodes;

  /*
  * Block processing control flags
  */
    typedef enum {
        BLOCK_REPLICATED_EVERY_FRAGMENT      = 1 << 0,
        BLOCK_PROCESS_FAIL_REPORT_REQUEST    = 1 << 1,
        BLOCK_DELETE_BUNDLE_IF_NOT_PROCESSED = 1 << 2,
        BLOCK_DISCARD_BLOCK_IF_NOT_PROCESSED = 1 << 4
    } BlockProcessingFlags;

private:
    uint8_t m_blockTypeCode;                // block type code
    uint32_t m_blockNumber;                 // block number
    uint64_t m_blockProcessingFlags;        // block processing control flags
    uint8_t m_crcType;                      // CRC Type
    uint32_t m_crcValue;                    // CRC of primary block
    String m_blockData;                     // block data

    // the canonical block, section 4.3.2, RFC 9171
    json m_canonical_block;                 // canonical block
};

} // namespace ns3

#endif /* BP_CANONICAL_BLOCK_H */
