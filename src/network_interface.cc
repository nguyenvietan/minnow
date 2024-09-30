#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

auto NetworkInterface::make_arp( const uint16_t opcode,
                                 const EthernetAddress& target_ethernet_address,
                                 const uint32_t target_ip_address ) const noexcept -> ARPMessage
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = ethernet_address_;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const AddressNumeric next_hop_numeric { next_hop.ipv4_numeric() };

  auto it = ip_2_eth_.find( next_hop_numeric );
  auto it2 = ip_2_timestamp_.find( next_hop_numeric );

  bool is_cache_expired = it != ip_2_eth_.end() && it->second.second + ARP_CACHE_TTL_ms <= ms_;
  bool is_pending_ended = it2 == ip_2_timestamp_.end() || it2->second + ARP_REPLY_TTL_ms_ <= ms_;

  if ( it == ip_2_eth_.end() || is_cache_expired ) {
    if ( !is_pending_ended )
      return;
    const ARPMessage arp_request { make_arp( ARPMessage::OPCODE_REQUEST, {}, next_hop_numeric ) };
    ip_2_datagram_[next_hop_numeric] = dgram;
    ip_2_timestamp_[next_hop_numeric] = ms_;
    transmit( { { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP }, serialize( arp_request ) } );
  } else {
    transmit( { it->second.first, ethernet_address_, EthernetHeader::TYPE_IPv4, serialize( dgram ) } );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ )
    return;

  IPv4Datagram ipv4_dgram;
  if ( parse( ipv4_dgram, frame.payload ) ) {
    datagrams_received_.emplace( ipv4_dgram );
    return;
  }

  ARPMessage arp_msg;
  if ( not parse( arp_msg, frame.payload ) )
    return;
  if ( arp_msg.opcode == ARPMessage::OPCODE_REPLY ) {
    ip_2_eth_[arp_msg.sender_ip_address] = { arp_msg.sender_ethernet_address, ms_ };
    auto it = ip_2_datagram_.find( arp_msg.sender_ip_address );
    if ( it != ip_2_datagram_.end() ) {
      transmit( { arp_msg.sender_ethernet_address,
                  ethernet_address_,
                  EthernetHeader::TYPE_IPv4,
                  serialize( it->second ) } );
    }
  } else if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST ) {
    if ( arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {
      const ARPMessage arp_msg_reply
        = make_arp( ARPMessage::OPCODE_REPLY, arp_msg.sender_ethernet_address, arp_msg.sender_ip_address );
      transmit( { arp_msg.sender_ethernet_address,
                  ethernet_address_,
                  EthernetHeader::TYPE_ARP,
                  serialize( arp_msg_reply ) } );
    }
  }

  // learn from ARP requests
  if ( ip_2_eth_.find( arp_msg.sender_ip_address ) == ip_2_eth_.end() ) {
    ip_2_eth_[arp_msg.sender_ip_address] = { arp_msg.sender_ethernet_address, ms_ };
  }
  ip_2_timestamp_.erase( arp_msg.sender_ip_address );
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  ms_ += ms_since_last_tick;
}
