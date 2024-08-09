#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t available_capacity = output_.writer().available_capacity();

  if ( output_.writer().is_closed() ||
      first_index > first_unassembled_index_ + available_capacity ||
      first_index + data.size() < first_unassembled_index_ ) {
    return;
  }

  if ( first_index + data.size() > first_unassembled_index_ + available_capacity ) {
    uint64_t exceed = first_index + data.size() - (first_unassembled_index_ + available_capacity);
    data.erase(data.size() - exceed);
    is_last_substring = false;
  }

  // now data should fit the stream
  if ( first_index <= first_unassembled_index_ ) {
    write_to_stream( first_index, data, is_last_substring );
  } else {
    cache_to_buffer( first_index, data, is_last_substring );
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_bytes_pending_;
}

void Reassembler::write_to_stream( uint64_t first_index, string data, bool is_last_substring ) {
  if ( first_unassembled_index_ > first_index ) {
    data.erase( 0, first_unassembled_index_ - first_index );
  }

  if ( buffer_.empty() ) {
    output_.writer().push(data);
    first_unassembled_index_ += data.size();
    if ( is_last_substring ) {
      output_.writer().close();
    }
    return;
  }

  uint64_t end_index = first_unassembled_index_ + data.size();
  auto it = buffer_.lower_bound( end_index );

  if ( it != buffer_.end() && it->first == end_index ) {
    output_.writer().push( data );
    output_.writer().push( it->second );
    auto it0 = buffer_.begin();
    while ( it0 != it ) {
      buffer_.erase( it0 );
      it0++;
    }
    buffer_.erase( it );
    first_unassembled_index_ += data.size() + it->second.size();
  } else {
    if ( it == buffer_.begin() ) {
      output_.writer().push( data );
      first_unassembled_index_ += data.size();
    } else {
      it--;
      std::string next = it->second.substr(0, end_index - it->first);
      output_.writer().push( data );
      output_.writer().push( next );
      first_unassembled_index_ += data.size() + next.size();
      auto it0 = buffer_.begin();
      while ( it0 != it ) {
        buffer_.erase( it0 );
        it0++;
      }
      buffer_.erase( it );
    }
  }
  
  if ( is_last_substring ) {
    output_.writer().close();
  }
}

void Reassembler::cache_to_buffer( uint64_t first_index, string data, bool is_last_substring ) {
  uint64_t end_index = first_index + data.size();

  if ( buffer_.empty() ) {
    buffer_[first_index] = data;
    total_bytes_pending_ += data.size();
    return;
  }

  uint64_t new_first_index = first_index;
  std::string conn = "";

  auto it = buffer_.begin(); 

  // find first segment
  while ( it != buffer_.end() && it->first + it->second.size() < first_index ) {
    it++;
  }

  if ( it == buffer_.end() ) {
    buffer_[first_index] = data;
    total_bytes_pending_ += data.size();
    return;
  }
  
  new_first_index = min(new_first_index, it->first );
  if ( it->first < first_index ) {
    conn += it->second.substr( 0, first_index - it->first );
  }
  conn += data;

  auto it_be = it;

  // find last segment
  while ( it != buffer_.end() && it->first + it->second.size() <= end_index ) {
    it++;
  }

  if ( it != buffer_.end() ) {
    if ( it->first <= end_index ) {
      conn += it->second.substr( end_index - it->first );
      it++;
    }
  }

  auto it_en = it;

  // clean up
  auto it_x = it_be;
  while ( it_x != it_en ) {
    auto tmp = it_x;
    total_bytes_pending_ -= it_x->second.size();
    it_x++;
    buffer_.erase( tmp );
  }

  // put concatenated strings
  buffer_[new_first_index] = conn;
  total_bytes_pending_ += conn.size();

  // TODO: need a wrapper BString { data, is_last_substring }
  if ( is_last_substring ) {
    output_.writer().close();
  }
}
