# General:
- Completion of the bundle-module created at the GSOC2013 (http://code.nsnam.org/dizhizhou/ns-3-dev-socis2013/summary) and updated to emulate Bundle Protocol v7
- Was developed to operate with NS-3 version 3.34 - has not been tested with any other version of NS-3

# How to Use:
- run from ns-allinone-3.34/ns-3.34/ with commands such as: 
  - `./waf --run bundle-protocol-simple`
  - `./waf --run bundle-protocol-nocla`
- Different branches have different capabilities, along with test scenarios to exercise them:
  - Using 'multi-hop' branch while receiving debug output from the TCP covergence layer:
    - `NS_LOG="BundleProtocol:BpTcpClaProtocol" ./waf --run bundle-protocol-multihop-tcp`
- Bundle Protocol supports two L4 protocols: TCP and LTP (LTP rides on top of UDP with a server port number of 1113)
  - Written to operate with NS-3 LTP module: https://github.com/alexk1vt/ns-3-ltp
- Nodes must have support L4 protocol installed prior to installing bundle protocol. To use LTP, nodes must have LTP installed and linked with all LTP ‘remote peers’ they will be communicating with.

# Bundle-Protocol:
## Configurable parameters:
- L4 type (“Ltp” and “Tcp” supported)
- BundleSize – maximum size of ADU before bundle fragmentation

## Addressing:
- Nodes are assigned bundle protocol endpoint IDs that are Uniform Resource Identifiers (URI) and follows the structure:
    “scheme”:”scheme-specific part (ssp)” (ie, “dtn”:”node0”)

## Routing:
- BP uses BpStaticRoutingProtocol class to look up ‘next hop’ for a given BP node
  - No static routes need to be added to the class if routing to node that is directly connected

## Classes and Methods of Interest:
```cpp
BpEndpointId::
  Uri () // Returns a string of endpoint id

BundleProtocolHelper::
  SetRoutingProtocol (Ptr<BpRoutingProtocol> rt);
  SetBpEndpointId (BpEndpointId eid);
  Install (NodeContainer c); // Installs BundleProtocol onto all nodes in the container

BpStaticRoutingProtocol::
  AddRoute (bpEndpointId dest, bpEndpointId nextHop); // Adds a route specifying the next bundle protocol endpoint Id hop in order to reach the given destination endpoint id

BundleProtocol::
  ExternalRegisterLtp (BpEndpointId eid, 
                       const double lifetime, 
                       const bool state, 
                       const InetSocketAddress L4Address); // Registers other nodes to the current node in order to send/receive bundles to/from them
  Std::vector<uint8_t> Receive_data (const BpEndpointId &eid) // Called to receive any bundles addressed to the provided endpoint id
  Send_data (const uint8_t *buffer, 
             const uint32_t data_size, 
             const BpEndpointId &src, 
             const BpEndpointId &dst); // Called to send data as a bundle from the designated source endpoint id to the designated destination endpoint id
```
## Constraints:
- Only supports singleton endpoint Ids at this time.
- Does not provide mechanisms to control bundle and block processing flags.
- Bundle Protocol Static Routing currently supports single routes.
  - Branch ‘alternate-static-bp-routes-merged’ has an implementation of alternate static routes that is not thoroughly tested.
- Some bundle data types are not RFC compliant, such as timestamps.
- No reports or administrative records are implemented.
- Bundle retention constraints are not implemented.
- No return forwarding or re-forwarding mechanisms implemented.
- No bundle expiration implemented.
- Only CRC-16 implemented for block checksum.
- No deferred/abandoned delivery options implemented.
- No transmission cancellation mechanism implemented.
- No rate limiting or congestion control mechanisms implemented.
- Unresolved memory leaks identified by Valgrind
