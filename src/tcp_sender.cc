#include <iostream>

#include "tcp_config.hh"
#include "tcp_sender.hh"

using namespace std;

void Timer::reset()
{
  ms_since_beginning_ = 0;
  cur_RTO_ms_ = initial_RTO_ms_;
}

void Timer::set_ms_since_begining( uint64_t x )
{
  ms_since_beginning_ = x;
}

void Timer::tick( uint64_t ms_since_last_tick )
{
  ms_since_beginning_ += ms_since_last_tick;
}

void Timer::double_RTO()
{
  cur_RTO_ms_ <<= 1;
}

bool Timer::is_timeout()
{
  return ms_since_beginning_ >= cur_RTO_ms_;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return cnt_seq_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return cnt_consecutive_retx_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  TCPSenderMessage msg {};

  auto& reader = input_.reader();
  auto& writer = input_.writer();

  for ( ;; ) {
    std::string payload = "";

    while ( !reader.peek().empty() && payload.size() < min( TCPConfig::MAX_PAYLOAD_SIZE, window_size_ ) ) {
      std::string_view content = reader.peek();
      std::string add = std::string( content );
      int diff = (int)payload.size() + content.size() - min( TCPConfig::MAX_PAYLOAD_SIZE, window_size_ );
      if ( diff >= 0 ) {
        add = add.substr( 0, add.size() - diff );
      }
      payload += add;
      reader.pop( add.size() );
    }

    if ( payload.empty() && sent_syn_ && !writer.is_closed() )
      break;

    if ( payload.empty() && writer.is_closed() && window_size_ == 0 ) // FIN flag occupies space in window
      break;

    if ( payload.empty() && sent_fin_ ) {
      break;
    }

    msg.SYN = !sent_syn_;
    msg.seqno = Wrap32::wrap( next_abs_seqno_, isn_ );
    msg.payload = payload;
    msg.FIN = writer.is_closed() && ( payload.size() + !sent_syn_ + cnt_seq_in_flight_ + 1 <= window_size_ );

    transmit( msg );

    sent_syn_ |= msg.SYN;
    sent_fin_ |= msg.FIN;

    buffer_.emplace( msg );
    next_abs_seqno_ += msg.sequence_length();
    cnt_seq_in_flight_ += msg.sequence_length();

    window_size_ -= msg.sequence_length();

    if ( window_size_ == 0 )
      break;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message {};
  message.seqno = Wrap32::wrap( next_abs_seqno_, isn_ );
  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( !msg.ackno.has_value() ) { // send_close.cc: "SYN + FIN"
    window_size_ = msg.window_size > 0 ? msg.window_size : window_size_;
    return;
  }

  uint64_t cur_abs_seqno = ( *msg.ackno ).unwrap( isn_, next_abs_seqno_ );

  if ( cur_abs_seqno > next_abs_seqno_ ) {
    return;
  }

  bool has_popped = false;
  while ( !buffer_.empty() ) {
    auto oldest_msg = buffer_.front();
    if ( oldest_msg.seqno.unwrap( isn_, next_abs_seqno_ ) >= cur_abs_seqno ) {
      break;
    }
    cnt_seq_in_flight_ -= oldest_msg.sequence_length();
    buffer_.pop();
    has_popped = true;
  }
  window_size_ = msg.window_size;
  if ( has_popped ) {
    timer.reset();
    cnt_consecutive_retx_ = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  timer.tick( ms_since_last_tick );
  if ( timer.is_timeout() && !buffer_.empty() ) {
    transmit( buffer_.front() );
    timer.double_RTO();
    timer.set_ms_since_begining( 0 );
    cnt_consecutive_retx_++;
  }
}
