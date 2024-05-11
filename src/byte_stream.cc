#include <iostream>
#include <vector>
#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if (Writer::is_closed() || Writer::available_capacity() == 0 || data.empty()) {
    return;
  }
  uint64_t remain = min(capacity_ - buffer_.size(), data.size());
  uint64_t i = 0;
  while (i < remain) {
    buffer_.push_back(data[i]);
    i++;
  }
  total_pushed_ += remain;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}

uint64_t Reader::bytes_popped() const
{
  return total_popped_;
}

string_view Reader::peek() const
{
  if (buffer_.empty()) return std::string_view {};
  std::string tmp = "";
  for (size_t i = 0; i < buffer_.size(); i++) {
    tmp += buffer_[i];
  }
  std::string_view out {tmp};
  return out;
}

void Reader::pop( uint64_t len )
{
  if (buffer_.empty()) return;
  uint64_t remain = min(len, buffer_.size());
  uint64_t i = 0;
  while (i < remain) {
    buffer_.pop_front();
    i++;
  }
  total_popped_ += remain;
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}
