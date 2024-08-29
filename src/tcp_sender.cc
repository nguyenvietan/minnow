#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return cnt_seq_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{
  auto& reader = input_.reader();

  std::string_view content = reader.peek();
  if ( content.empty() && sent_syn_ )
    return;


  TCPSenderMessage msg {};
  msg.payload = content;
  msg.SYN = !sent_syn_;
  msg.seqno = Wrap32::wrap( next_abs_seqno_, isn_ );

  transmit( msg );

  sent_syn_ |= true;

  buffer_.emplace( msg );
  next_abs_seqno_ += msg.sequence_length();
  cnt_seq_in_flight_ += msg.sequence_length();

  reader.pop( content.size() );
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message {};
  message.seqno = Wrap32::wrap( next_abs_seqno_, isn_ );
  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  uint64_t cur_abs_seqno = ( *msg.ackno ).unwrap( isn_, next_abs_seqno_ );
  while ( !buffer_.empty() ) {
    auto oldest_msg = buffer_.front();
    if ( oldest_msg.seqno.unwrap( isn_, next_abs_seqno_ ) >= cur_abs_seqno ) {
      break;
    }
    cnt_seq_in_flight_ -= oldest_msg.sequence_length();
    buffer_.pop();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}
