#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  routing_table_[prefix_length][route_prefix] = { interface_num, next_hop };
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( const auto& interface : _interfaces ) {
    auto&& datagram_received { interface->datagrams_received() };
    while ( not datagram_received.empty() ) {
      auto datagram { move( datagram_received.front() ) };
      datagram_received.pop();
      if ( datagram.header.ttl <= 1 ) {
        continue;
      }
      datagram.header.ttl -= 1;
      datagram.header.compute_checksum();

      const auto& mp { match( datagram.header.dst ) };
      if ( not mp.has_value() ) {
        continue;
      }
      const auto& [num, next_hop] { mp.value() };
      _interfaces[num]->send_datagram( datagram,
                                       next_hop.value_or( Address::from_ipv4_numeric( datagram.header.dst ) ) );
    }
  }
}

std::optional<Router::info> Router::match( uint32_t ip_addr )
{
  for ( int prefix_length = 31; prefix_length >= 0; prefix_length-- ) {
    auto m = routing_table_[prefix_length];
    uint32_t mask = ~(uint32_t)( ( static_cast<uint64_t>( 1 ) << ( 32 - prefix_length ) ) - 1 );
    auto it = m.find( mask & ip_addr );
    if ( it != m.end() ) {
      return it->second;
    }
  }
  return {};
}
