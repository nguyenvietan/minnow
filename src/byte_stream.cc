#define barrier() __asm__ __volatile__( "" : : : "memory" )
#include "byte_stream.hh"
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : preview(), stream_(), capacity_( capacity ), available_capacity_( capacity ), error_( false )
{
  preview = ""sv;
}

bool Writer::is_closed() const
{
  // 返回文件流是否关闭
  return is_closed_;
}

void Writer::push( string data ) noexcept
{
  // Your code here.
  if ( is_closed_ || available_capacity_ <= 0 || !data.length() ) {
    return;
  }
  if ( data.length() > available_capacity() ) {
    data.resize( available_capacity() );
  }
  available_capacity_ -= data.length();
  total_bytes_pushed_ += data.length();
  stream_.emplace( move( data ) );
  if ( !stream_.empty() && preview.empty() ) {
    preview = stream_.front();
  }
  return;
}

void Writer::close()
{
  // Your code here.
  if ( !is_closed_ ) {
    is_closed_ = true;
    // stream_.emplace( string( 1, EOF ) );
  }
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return available_capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return total_bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return available_capacity_ == capacity_ && is_closed_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return total_bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  // return {};
  // *str =  string(1, stream_.front());
  // return string_view(*str);
  // if(stream_.empty())
  // {
  //   preview = empty_str;
  // }
  return preview;
  // return string_view(string(stream_.begin(), stream_.end()));
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( len > capacity_ - available_capacity_ ) {
    return;
  }
  available_capacity_ += len; // pop了len 所以可用空间多len
  total_bytes_popped_ += len;
  while ( len > 0 ) {
    if ( len >= preview.size() ) {
      len -= preview.size();
      stream_.pop();
      if ( stream_.empty() ) {
        preview = ""sv;
      } else {
        preview = stream_.front();
      }
    } else {
      preview.remove_prefix( len );
      len = 0;
    }
    if ( preview.size() == 0 ) {
      break;
    }
  }
  // if ( len >= preview.size() ) {
  //   stream_.pop();
  //   preview = stream_.empty() ? ""sv : stream_.front();
  // } else
  //   preview.remove_prefix( len );
  // available_capacity_ += len;
  // total_bytes_popped_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return capacity_ - available_capacity_;
}