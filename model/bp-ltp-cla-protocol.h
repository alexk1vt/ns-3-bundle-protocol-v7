/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef BP_LTP_CLA_PROTOCOL_H
#define BP_LTP_CLA_PROTOCOL_H

#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "bp-cla-protocol.h"
#include "bp-endpoint-id.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "bundle-protocol.h"
#include "bp-routing-protocol.h"
#include "bp-bundle.h"

#include "ns3/ltp-protocol-helper.h"
#include "ns3/ltp-protocol.h"

#include <map>

namespace ns3 {

class Node;

/**
 * \ingroup bundle-ltp-cla
 *
 * \brief Bundle Protocol LTP convergence layer agent
 *
 * This class implements the Bundle Protocol LTP convergence layer agent.
 * It is a subclass of the abstract base class BpClaProtocol.
 */
class BpLtpClaProtocol : public BpClaProtocol
{
public:
    static TypeId GetTypeId (void);

    /**
     * \brief Constructor
     */
    BpLtpClaProtocol ();
    
    /**
     * \brief Destructor
     */
    virtual ~BpLtpClaProtocol ();

    /**
     * send bundle to the transport layer
     * 
     * \param bundle the bundle to send
    */
    virtual int SendBundle (Ptr<BpBundle> bundle);

    /**
     * 
    */
}

} // namespace ns3

#endif /* BP_TCP_LTP_PROTOCOL_H */