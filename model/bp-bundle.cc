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
BpBundle::BpBundle ()
  : m_retentionConstraint (0),
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
}

BpBundle::BpBundle (uint8_t constraint, BpPrimaryBlock primaryBlock, BpCanonicalBlock payloadBlock)
    : m_retentionConstraint (constraint)
{
    NS_LOG_FUNCTION (this);
    // build the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
    m_bundle["primary_block"] = primaryBlock.GetJson();
    m_bundle["payload_block"] = payloadBlock.GetJson();
}

BpBundle::~BpBundle ()
{
    NS_LOG_FUNCTION (this);
}

bool
BpBundle::IsDispatchPending () const
{
    NS_LOG_FUNCTION (this);
    BundleRetentionConstraint constraint = BUNDLE_DISPATCH_PENDING
    return m_retentionConstraint & constraint;
}

bool
BpBundle::IsForwardPending () const
{
    NS_LOG_FUNCTION (this);
    BundleRetentionConstraint constraint = BUNDLE_FORWARD_PENDING
    return m_retentionConstraint & constraint;
}

bool
BpBundle::IsReassemblyPending () const
{
    NS_LOG_FUNCTION (this);
    BundleRetentionConstraint constraint = BUNDLE_REASSEMBLY_PENDING
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
BpBundle::GetExtensionBlock () const
{
    NS_LOG_FUNCTION (this);
    return m_extensionBlock;  // TODO:  implement support for multiple extension blocks
}

uint8_t
BpBundle::GetExtensionBlockCount () const
{
    NS_LOG_FUNCTION (this);
    return 1;       // TODO:  correct this once support for multiple extension blocks is implemented
}

std::vector <std::uint8_t>
BpBundle::GetCborEncoding () const
{
    NS_LOG_FUNCTION (this);
    return json.to_cbor(m_bundle);
}

void
BpBundle::SetRetentionConstraint (uint8_t constraint)
{
    NS_LOG_FUNCTION (this << static_cast<uint16_t> (constraint));
    m_retentionConstraint = constraint;
    m_bundle["retention_constraint"] = m_retentionConstraint;
}

void
BpBundle::SetPrimaryBlock (BpPrimaryBlock &primaryBlock)
{
    NS_LOG_FUNCTION (this);
    m_primaryBlock = primaryBlock;
    m_bundle["primary_block"] = m_primaryBlock.GetJson();
}

void
BpBundle::SetPayloadBlock (BpCanonicalBlock &payloadBlock)
{
    NS_LOG_FUNCTION (this);
    m_payloadBlock = payloadBlock;
    m_bundle["payload_block"] = m_payloadBlock.GetJson();
}

void
BpBundle::SetExtensionBlock (BpCanonicalBlock &extensionBlock)
{
    NS_LOG_FUNCTION (this);
    m_extensionBlock = extensionBlock;
    m_bundle["extension_block"] = m_extensionBlock.GetJson();
}

void
BpBundle::RebuildBundle ()
{
    NS_LOG_FUNCTION (this);
    // rebuild the json bundle
    m_bundle["retention_constraint"] = m_retentionConstraint;
    m_bundle["primary_block"] = primaryBlock.GetJson();
    m_bundle["payload_block"] = payloadBlock.GetJson();
}

void
BpBundle::SetBundleFromCbor (std::vector <std::uint8_t> cborBundle)
{
    NS_LOG_FUNCTION (this);
    m_bundle = json::from_cbor(cborBundle);
    m_retentionConstraint = m_bundle["retention_constraint"];
    m_primaryBlock.SetJson(m_bundle["primary_block"]);
    m_payloadBlock.SetJson(m_bundle["payload_block"]);
    if (GetExtensionBlockCount() > 0)
    {
        m_extensionBlock.SetJson(m_bundle["extension_block"]); // TODO - implement support for multiple extension blocks
    }
}

} // namespace ns3
