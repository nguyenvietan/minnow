#include "tcp_receiver.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }

  uint64_t checkpoint = reassembler_.writer().bytes_pushed();

  if ( message.SYN ) {
    if ( !ISN_.has_value() ) {
      ISN_ = message.seqno;
    } else {
      return;
    }
  } else {
    if ( ISN_.has_value() && *ISN_ == message.seqno ) {
      return;
    }
  }

  uint64_t first_index = message.seqno.unwrap( *ISN_, checkpoint ) - !( *ISN_ == message.seqno );

  reassembler_.insert( first_index, std::move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message;

  message.window_size
    = static_cast<uint16_t>( min( reassembler_.writer().available_capacity(), static_cast<uint64_t>( 65535 ) ) );

  uint64_t checkpoint = reassembler_.writer().bytes_pushed();
  if ( ISN_.has_value() )
    message.ackno = Wrap32::wrap( checkpoint, *ISN_ ) + ( 1 + reassembler_.writer().is_closed() );

  message.RST = reassembler_.writer().has_error();

  return message;
}
