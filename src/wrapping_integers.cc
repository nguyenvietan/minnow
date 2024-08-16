#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point + ( n & 0xffffffff ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t mod = static_cast<uint64_t>( 1 ) << 32;
  uint32_t dist = raw_value_ - wrap( checkpoint, zero_point ).raw_value_;
  if ( dist <= mod / 2 || dist + checkpoint < mod ) {
    return checkpoint + dist;
  }
  return checkpoint + dist - mod;
}
