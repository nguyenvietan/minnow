#include "reassembler.hh"

using namespace std;

void Reassembler::flush()
{
  auto it = buffer_.begin();
  while ( it != buffer_.end() && !output_.writer().is_closed() && output_.writer().available_capacity() > 0 ) {
    if ( it->first > expectedIdx_ ) {
      break;
    }
    expectedIdx_ += it->second.data.size();
    total_bytes_pending_ -= it->second.data.size();
    output_.writer().push( it->second.data );
    if ( it->second.is_last_substring ) {
      total_bytes_pending_ = 0;
      output_.writer().close();
    }
    auto tmp = it;
    it++;
    buffer_.erase( tmp );
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;
  const uint64_t unexpectedIdx = expectedIdx_ + output_.writer().available_capacity();
  if ( output_.writer().is_closed() || output_.writer().available_capacity() <= 0
       || first_index >= unexpectedIdx ) {
    return;
  }
  if ( first_index + data.size() > unexpectedIdx ) {
    data.resize( unexpectedIdx - first_index );
    is_last_substring = false;
  }
  if ( first_index <= expectedIdx_ ) {
    push_to_stream( first_index, data, is_last_substring );
  } else {
    cache_to_buffer( first_index, data, is_last_substring );
  }
  flush();
}

void Reassembler::push_to_stream( uint64_t first_index, string data, bool is_last_substring )
{
  // 先考虑掐头
  if ( first_index < expectedIdx_ ) {
    data.erase( 0, expectedIdx_ - first_index );
    first_index = expectedIdx_;
  }
  // 首先string.resize()开销一定比erase()小, 所以尽可能的避免掐头
  // 考虑到flush中首部string有可能与data有重叠, 所以应该将可能存在的重叠在data中通过resize()删除
  if ( !buffer_.empty() ) {
    auto it = buffer_.begin();
    auto ite = buffer_.lower_bound( first_index + data.size() );
    int flag = 1;
    while ( flag && it != buffer_.end() ) {
      flag = ( it != ite );
      if ( first_index + data.size() > it->first + it->second.data.size() ) {
        total_bytes_pending_ -= it->second.data.size();
        auto tmp = it;
        it++;
        buffer_.erase( tmp );
        continue;
      }
      // 只是有重叠, 没有全覆盖, 注意这里不存在first_index == it->first的情况
      else if ( first_index + data.size() > it->first ) {
        data.resize( it->first - first_index );
      } else {
        break;
      }
      it++;
    }
  }
  expectedIdx_ += data.size();
  output_.writer().push( std::move( data ) );
  if ( is_last_substring ) {
    output_.writer().close();
  }
}

void Reassembler::cache_to_buffer( uint64_t first_index, string data, bool is_last_substring )
{
  auto it3 = buffer_.lower_bound( first_index + data.size() );
  auto it2 = buffer_.lower_bound( first_index );
  auto it1 = it2;
  // data期望插入位置是pos, 如果pos不是首部, 则要考虑buffer[pos - 1]的data需要resize()
  if ( it2 != buffer_.begin() ) {
    it1--;
    if ( it1->first + it1->second.data.size() > first_index ) {
      // 交叠
      if ( it1->first + it1->second.data.size() < first_index + data.size() ) {
        total_bytes_pending_ -= it1->second.data.size();
        total_bytes_pending_ += first_index - it1->first;
        it1->second.data.resize( first_index - it1->first );
      }
      // 全覆盖
      else {
        return; // do nothing
      }
    }
  }
  if ( it2 == buffer_.end() ) {
    // getchar();
    total_bytes_pending_ += data.size();
    BString tmp( std::move( data ), is_last_substring );
    buffer_[first_index] = std::move( tmp );
    return;
  }
  auto it = it2;
  int flag = 1;
  while ( flag && it != buffer_.end() ) {
    flag = ( it != it3 );
    // 考虑it->data被全覆盖的情况
    if ( first_index + data.size() > it->first + it->second.data.size() ) {
      total_bytes_pending_ -= it->second.data.size();
      auto tmp = it;
      it++;
      buffer_.erase( tmp );
      continue;
    }
    // 考虑交叠情况
    else if ( first_index + data.size() > it->first ) {
      data.resize( it->first - first_index );
      is_last_substring = false;
    }
    // 无重叠可以直接离开循环
    else {
      break;
    }
    it++;
  }
  if ( data.size() == 0 ) {
    return;
  }
  total_bytes_pending_ += data.size();
  BString tmp( std::move( data ), is_last_substring );
  buffer_[first_index] = std::move( tmp );
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return total_bytes_pending_;
}
