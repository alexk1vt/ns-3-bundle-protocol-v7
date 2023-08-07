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
 */

#include "bp-static-routing-protocol.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("BpStaticRoutingProtocol");

namespace ns3 {

TypeId 
BpStaticRoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BpStaticRoutingProtocol")
    .SetParent<BpRoutingProtocol> ()
    .AddConstructor<BpStaticRoutingProtocol> ()
  ;
  return tid;
}

BpStaticRoutingProtocol::BpStaticRoutingProtocol ()
  : m_bp (0)
{ 
  NS_LOG_FUNCTION (this);
}

BpStaticRoutingProtocol::~BpStaticRoutingProtocol ()
{ 
  NS_LOG_FUNCTION (this);

  m_bp = 0;

  m_routeMap.clear ();
}

void
BpStaticRoutingProtocol::SetBundleProtocol (Ptr<BundleProtocol> bundleProtocol)
{ 
  NS_LOG_FUNCTION (this << " " << bundleProtocol);
  //m_bp->Unref();
  m_bp = bundleProtocol;
}

int 
BpStaticRoutingProtocol::AddRoute (BpEndpointId eid, BpEndpointId next_hop) // -- setting the next hop node for target node
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri () << " " << next_hop.Uri ());
  std::map <BpEndpointId, BpEndpointId>::iterator it = m_routeMap.find (eid);
  if (it == m_routeMap.end ())
    {
      m_routeMap.insert (std::pair<BpEndpointId, BpEndpointId>(eid, next_hop));
    }
  else
    {
      // duplicate routing
      return -1;
    }

  return 0;
}

//InetSocketAddress 
BpEndpointId
BpStaticRoutingProtocol::GetRoute (BpEndpointId eid)
{ 
  NS_LOG_FUNCTION (this << " " << eid.Uri ());
  std::map <BpEndpointId, BpEndpointId>::iterator it = m_routeMap.find (eid);
  if (it == m_routeMap.end ())
    {
      return eid;
    }
  else
    {
      return (*it).second;
    }
}

} // namespace ns3
