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
#ifndef BP_BUNDLE_H
#define BP_BUNDLE_H

#include <stdint.h>
#include <string>
#include <ctime>
#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/buffer.h"
#include "ns3/sequence-number.h"
#include "bp-endpoint-id.h"
#include "bp-primary-block.h"
#include "bp-canonical-block.h
#include "json.hpp"
using json = nlohmann::json;

namespace ns3 {

/**
 * \brief Bundle protocol bundle
 * 
 * The structure of the bundle protocol bundle, which is defined in section 4.3.1 of RFC 9171.
 * 
*/
class BpBundle
{
public:
  BpBundle ();
  virtual ~BpBundle ();

  // Setters
    /**
     * \brief Set the bundle constraint
     * 
     * \param constraint the bundle constraint
     */
    void SetRetentionConstraint (uint8_t constraint);

    /**
     * \brief Set the bundle primary block
    */
    void SetPrimaryBlock (const BpPrimaryBlock &primaryBlock);

    /**
     * \brief Set the bundle extension block
    */
    void SetExtensionBlock (const BpCanonicalBlock &extensionBlock);

    /**
     * \brief Set the bundle payload block
    */
    void SetPayloadBlock (const BpCanonicalBlock &payloadBlock);

    /**
     * \brief Rebuild bundle with currently configured values
    */
    void RebuildBundle ();

    /**
     * \brief Deserialize from CBOR encoding
     * 
     * \param cborEncoding the bundle CBOR encoding
     */
    void SetBundleFromCbor (const std::vector <std::uint8_t> cborEncoding);
    

  // Getters

    /**
     * \brief Get the bundle retentioin constraint
     * 
     * \return the bundle retention constraint
     */
    uint8_t GetRetentionConstraint () const;

    /**
     * \brief Get the bundle primary block
     * 
     * \return the bundle primary block
     */
    BpPrimaryBlock GetPrimaryBlock () const;

    /**
     * \brief Get a pointer to the bundle primary block
     * 
     * \return a pointer to the bundle primary block
     */
    BpPrimaryBlock *GetPrimaryBlockPtr ();

    /**
     * \brief Get a pointer to the payload block
     * 
     * \return a pointer to the payload block
     */
    BpCanonicalBlock *GetPayloadBlockPtr ();

    /**
     * \brief Get the bundle extension block
     * 
     * \return the bundle extension block  // TODO:  change implementation to support several extension blocks
     */
    BpCanonicalBlock GetExtensionBlock () const;

    /**
     * \brief Get the number of bundle extension blocks
     * 
     * \return the number of bundle extension blocks
     */
    uint8_t GetExtensionBlockCount () const;

    /**
     * \brief Get the bundle payload block
     * 
     * \return the bundle payload block
     */
    BpCanonicalBlock GetPayloadBlock () const;

    /**
     * \brief Serialize to CBOR encoding
     * 
     * \return the bundle CBOR encoding
     */
    std::vector <std::uint8_t> GetCborEncoding () const;

    // TODO:  Add the rest of the getters

  /*
  * Bundle constraints
  */
    typedef enum {
        BUNDLE_DISPATCH_PENDING    = 1 << 0,
        BUNDLE_FORWARD_PENDING     = 1 << 1,
        BUNDLE_REASSEMBLY_PENDING  = 1 << 2,
    } BundleRetentionConstraint;

private:
    uint8_t m_retentionConstraint;          // the version of bundle protocol
    BpPrimaryBlock m_primaryBlock;          // primary block
    BpCanonicalBlock m_extensionBlock;      // extension block  // TODO:  change implementation to support several extension blocks
    BpCanonicalBlock m_payloadBlock;        // payload block

    // bundle, section 4.3, RFC 9171
    json m_bundle;            // bundle
    std::vector <std::uint8_t> m_bundle_cbor_encoding; // bundle in CBOR encoding

};

} // namespace ns3

#endif /* BP_BUNDLE_H */
