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
#include "bp-canonical-block.h"
#include "json.hpp"
using json = nlohmann::json;

namespace ns3 {

void PrintVectorBuffer (std::vector <std::uint8_t> v_cbor);
void CompareCborBytesToVector (std::vector <std::uint8_t> v_cbor1, std::vector <std::uint8_t> v_cbor2);
void PrintCborBytes (std::vector <std::uint8_t> cborBundle);

/**
 * \brief Bundle protocol bundle header
 * 
 * A quick way to account for NS-3 not seperating packets in the receive buffer
 * 
*/
class BpBundleHeader : public Header
{
public:
  BpBundleHeader ();
  virtual ~BpBundleHeader ();

  // Setters
  
  /**
   * \brief Set the size of the bundle
   * 
   * \param size the size of the bundle
   */
  void SetBundleSize (const uint32_t size);

  // Getters

  /**
   * \brief Get the size of the bundle
   * 
  */
  uint32_t GetBundleSize () const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  private:
    uint32_t m_bundleSize;          // the bundle size
};

/**
 * \brief Bundle protocol bundle
 * 
 * The structure of the bundle protocol bundle, which is defined in section 4.3.1 of RFC 9171.
 * 
*/
class BpBundle : public Object // SimpleRefCount<BpBundle>
{
public:
  BpBundle ();
  BpBundle (uint8_t constraint, BpPrimaryBlock primaryBlock, BpCanonicalBlock payloadBlock);
  BpBundle (std::vector <uint8_t> cborBundle);
  BpBundle (uint8_t constraint, BpEndpointId src, BpEndpointId dst, uint32_t payloadSize, const uint8_t* payloadData);
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
    int AddExtensionBlock (const BpCanonicalBlock &extensionBlock);

    int AddUpdateExtensionBlock (const BpCanonicalBlock &extensionBlock);
    int AddUpdateExtensionBlock (uint8_t blockType, BpEndpointId eid);
    int AddUpdateExtensionBlock (uint8_t blockType);
    int AddUpdateExtensionBlock (uint8_t blockType, uint64_t blockProcessingFlags, uint8_t crcType, std::string blockData);

    int AddUpdatePrevNodeExtBlock (BpEndpointId eid);
    std::string GetPrevNodeExtBlockData ();
    int AddUpdateBundleAgeExtBlock ();
    uint64_t GetBundleAgeExtBlockData ();
    int AddUpdateHopCountExtBlock ();
    uint64_t GetHopCountExtBlockData ();

    bool HasExtensionBlock (uint8_t blockType);

    /**
     * \brief Set the bundle payload block
    */
    void SetPayloadBlock (const BpCanonicalBlock &payloadBlock);

    /**
     * \brief Rebuild bundle with currently configured values
    */
    //void RebuildBundle ();

    /**
     * \brief Deserialize from CBOR encoding
     * 
     * \param cborEncoding the bundle CBOR encoding
     */
    int SetBundleFromCbor (const std::vector <std::uint8_t> cborEncoding);
    
    int AddToBundle(uint8_t blockIndex, BpCanonicalBlock block);
    int AddBlocksFromBundle(Ptr<BpBundle> donatingBundle);
    int AddBlocksFromBundle(Ptr<BpBundle> donatingBundle, BpCanonicalBlock::CanonicalBlockTypeCodes blockType, bool headlessBundle);
    int AddBlocksFromBundleExcept(Ptr<BpBundle> donatingBundle, BpCanonicalBlock::CanonicalBlockTypeCodes blockType, bool headlessBundle);
    int SetBundleFromJson (json donatingJson);

    int AddCRCToBundle(uint8_t crcType);    

  // Getters

    /**
     * \brief Is the bundle pending dispatch?
     * 
     * \return true if the bundle is pending dispatch, false otherwise
     */
    bool IsDispatchPending () const;

    /**
     * \brief Is the bundle pending forward?
     * 
     * \return true if the bundle is pending forward, false otherwise
     */
    bool IsForwardPending () const;

    /**
     * \brief Is the bundle pending reassembly?
     * 
     * \return true if the bundle is pending reassembly, false otherwise
     */
    bool IsReassemblyPending () const;

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
    BpPrimaryBlock *GetPrimaryBlockPtr ()
    {
        return &m_primaryBlock;
    }

    /**
     * \brief Get a pointer to the payload block
     * 
     * \return a pointer to the payload block
     */
    BpCanonicalBlock *GetPayloadBlockPtr ()
    {
        return &m_payloadBlock;
        //BpCanonicalBlock payloadBlock;
        //payloadBlock.SetCanonicalBlockFromJson(m_bundle[BUNDLE_PAYLOAD_BLOCK]);
        //m_payloadBlock = payloadBlock;
        //return &m_payloadBlock;
    }

    json GetJson ()
    {
      return m_bundle;
    }

    /**
     * \brief Get the bundle extension block
     * 
     * \return the bundle extension block  // TODO:  change implementation to support several extension blocks
     */
    BpCanonicalBlock GetExtensionBlock (uint8_t blockType);

    /**
     * \brief Get the number of bundle extension blocks
     * 
     * \return the number of bundle extension blocks
     */
    uint8_t GetExtensionBlockCount ();

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
    std::vector <std::uint8_t> GetCborEncoding ();

    // Functions for adding extension blocks to the bundle
    uint8_t GetNewBlockNumber ();
    //uint8_t GetNewBlockIndex ();

    /**
     * \brief Get the size of the CBOR encoded bundle in bytes
     * 
     * \return the size of the CBOR encoded bundle in bytes
     */
    uint GetCborEncodingSize ();
    void PrintCborBytes ();
    bool empty () const;
    std::string BlockNumberToString (uint8_t blockNumber);

    /*
    * CRC operations
    */
    int AddCrcToBundle(uint8_t crcType);

    // TODO:  Add the rest of the getters

    /*
    * Iterator operations
    */
    json::iterator begin ();
    json::iterator end ();

    /*
    * Inline operators
    */
    inline void operator << (const BpBundle &bundle) const
    {
      
    }


  /*
  * Bundle constraints
  */
    typedef enum {
        BUNDLE_DISPATCH_PENDING    = 1 << 0,
        BUNDLE_FORWARD_PENDING     = 1 << 1,
        BUNDLE_REASSEMBLY_PENDING  = 1 << 2,
    } BundleRetentionConstraint;

  /*
  * Bundle Block Index values
  */
 //typedef enum {
 //   BUNDLE_PRIMARY_BLOCK = 0,
 //   BUNDLE_PAYLOAD_BLOCK = 255
 //} BundleBlockIndex;

  std::string BUNDLE_PRIMARY_BLOCK = "000";
  std::string BUNDLE_PAYLOAD_BLOCK = "255";

private:
    uint8_t m_retentionConstraint;          // the bundle retention constraint
    BpPrimaryBlock m_primaryBlock;          // primary block
    BpCanonicalBlock m_extensionBlock;      // extension block  // TODO:  change implementation to support several extension blocks
    BpCanonicalBlock m_payloadBlock;        // payload block

    // bundle, section 4.3, RFC 9171
    json m_bundle;            // bundle
    std::vector <std::uint8_t> m_bundle_cbor_encoding; // bundle in CBOR encoding

};

} // namespace ns3

#endif /* BP_BUNDLE_H */
