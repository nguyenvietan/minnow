#define barrier() __asm__ __volatile__( "" : : : "memory" )
#include "byte_stream.hh"
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return close_;
}

void Writer::push( string data ) noexcept
{
    if (close_ || available_capacity() == 0 || data.empty()) return;
    size_t L = min(available_capacity(), data.size());
    deque_.push_back(data.substr(0, L));
    total_bytes_pushed += L;
    bytes_in_buffer += L;
}

void Writer::close()
{
  close_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - bytes_in_buffer;
}

uint64_t Writer::bytes_pushed() const
{
  return total_bytes_pushed;
}

bool Reader::is_finished() const
{
  return close_ && deque_.empty();
}

uint64_t Reader::bytes_popped() const
{
  return total_bytes_popped;
}

string_view Reader::peek() const
{
  return string_view { deque_.front() };
}

void Reader::pop( uint64_t len )
{
  uint64_t bytes_popped = 0;
  for (; bytes_popped < len; ) {
    string tmp = deque_.front();
    if (bytes_popped + tmp.size() <= len) {
      deque_.pop_front();
      bytes_popped += tmp.size();
      bytes_in_buffer -= tmp.size();
    } else {
      uint64_t remaining = bytes_popped + tmp.size() - len;
      string x = tmp.substr(tmp.size() - remaining);
      deque_.pop_front();
      deque_.push_front(x);
      bytes_in_buffer -= tmp.size() - remaining;
      bytes_popped += tmp.size() - remaining;
    }
  }  
  total_bytes_popped += bytes_popped;
}

uint64_t Reader::bytes_buffered() const
{
  return bytes_in_buffer;
}
