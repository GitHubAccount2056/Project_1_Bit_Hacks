/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

// Implements the ADT specified in bitarray.h as a packed array of bits; a bit
// array containing bit_sz bits will consume roughly bit_sz/8 bytes of
// memory.


#include "./bitarray.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>


// ********************************* Types **********************************

// Concrete data type representing an array of bits.
struct bitarray {
  // The number of bits represented by this bit array.
  // Need not be divisible by 8.
  size_t bit_sz;

  // The underlying memory buffer that stores the bits in
  // packed form (8 per byte).
  char* buf;
};


// ******************** Prototypes for static functions *********************

// Rotates a subarray left by an arbitrary number of bits.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
// bit_left_amount is the number of places to rotate the
//                    subarray left
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
static inline void bitarray_rotate_right(bitarray_t* const bitarray,
                                 const size_t bit_offset,
                                 const size_t bit_length,
                                 const size_t bit_left_amount);

// Portable modulo operation that supports negative dividends.
//
// Many programming languages define modulo in a manner incompatible with its
// widely-accepted mathematical definition.
// http://stackoverflow.com/questions/1907565/c-python-different-behaviour-of-the-modulo-operation
// provides details; in particular, C's modulo
// operator (which the standard calls a "remainder" operator) yields a result
// signed identically to the dividend e.g., -1 % 10 yields -1.
// This is obviously unacceptable for a function which returns size_t, so we
// define our own.
//
// n is the dividend and m is the divisor
//
// Returns a positive integer r = n (mod m), in the range
// 0 <= r < m.
static inline size_t modulo(const ssize_t n, const size_t m);

// Produces a mask which, when ANDed with a byte, retains only the
// bit_index th byte.
//
// Example: bitmask(5) produces the byte 0b00100000.
//
// (Note that here the index is counted from right
// to left, which is different from how we represent bitarrays in the
// tests.  This function is only used by bitarray_get and bitarray_set,
// however, so as long as you always use bitarray_get and bitarray_set
// to access bits in your bitarray, this reverse representation should
// not matter.
static inline char bitmask(const size_t bit_index);


// ******************************* Functions ********************************

bitarray_t* bitarray_new(const size_t bit_sz) {
  // Allocate an underlying buffer of ceil(bit_sz/8) bytes.
  char* const buf = calloc(1, (bit_sz+7) / 8);
  if (buf == NULL) {
    return NULL;
  }

  // Allocate space for the struct.
  bitarray_t* const bitarray = malloc(sizeof(struct bitarray));
  if (bitarray == NULL) {
    free(buf);
    return NULL;
  }

  bitarray->buf = buf;
  bitarray->bit_sz = bit_sz;
  return bitarray;
}

void bitarray_free(bitarray_t* const bitarray) {
  if (bitarray == NULL) {
    return;
  }
  free(bitarray->buf);
  bitarray->buf = NULL;
  free(bitarray);
}

size_t bitarray_get_bit_sz(const bitarray_t* const bitarray) {
  return bitarray->bit_sz;
}

inline bool bitarray_get(const bitarray_t* const restrict bitarray, const size_t bit_index) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to get the nth
  // bit, we want to look at the (n mod 8)th bit of the (floor(n/8)th)
  // byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to produce either a zero byte (if the bit was 0) or a nonzero byte
  // (if it wasn't).  Finally, we convert that to a boolean.
  return (bitarray->buf[bit_index / 8] & bitmask(bit_index)) ?
         true : false;
}

inline void bitarray_set(bitarray_t* const restrict bitarray,
                  const size_t bit_index,
                  const bool value) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to set the nth
  // bit, we want to set the (n mod 8)th bit of the (floor(n/8)th) byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to clear out the bit we're about to set.  We bitwise-or the result
  // with a byte that has either a 1 or a 0 in the correct place.
  bitarray->buf[bit_index / 8] =
    (bitarray->buf[bit_index / 8] & ~bitmask(bit_index)) |
    (value ? bitmask(bit_index) : 0);
}

void bitarray_randfill(bitarray_t* const bitarray){
  int32_t *ptr = (int32_t *)bitarray->buf;
  for (int64_t i=0; i<bitarray->bit_sz/32 + 1; i++){
    ptr[i] = rand();
  }
}

// Get 64 bits from bitstring
inline uint64_t bitarray_get_u64(const bitarray_t *b, size_t bit_index) {
    size_t byte_idx = bit_index / 8;
    size_t bit_off  = bit_index % 8;
    uint64_t low_word;
    memcpy(&low_word, (b -> buf) + byte_idx, sizeof(uint64_t));

    if (bit_off == 0) {
        return low_word;
    }

    uint8_t high_byte = (uint8_t) (b -> buf[byte_idx + sizeof(uint64_t)]);

    return (low_word >> bit_off) | (((uint64_t) high_byte) << (64 - bit_off));
}

// Set 64 bits in bitstring
inline void bitarray_set_u64(bitarray_t *b, size_t bit_index, uint64_t value) {
    size_t byte_idx = bit_index / 8;
    size_t bit_off  = bit_index % 8;

    if (bit_off == 0) {
        memcpy(b -> buf + byte_idx, &value, sizeof(uint64_t));
        return;
    }

    uint64_t old_word;
    memcpy(&old_word, (b -> buf) + byte_idx, sizeof(uint64_t));
    
    uint8_t old_byte = (uint8_t) b-> buf[byte_idx + sizeof(uint64_t)];

    uint64_t mask_lo = (1ULL << bit_off) - 1; 
    uint8_t mask_hi  = ~((1U << bit_off) - 1);

    uint64_t new_word = (old_word & mask_lo) | (value << bit_off);
    uint8_t new_byte  = (old_byte & mask_hi) | (uint8_t)(value >> (64 - bit_off));

    memcpy(b -> buf + byte_idx, &new_word, sizeof(uint64_t));
    b -> buf[byte_idx + sizeof(uint64_t)] = (char)new_byte;
}

static inline void 
bitarray_reverse_range(bitarray_t* const restrict bitarray, const size_t start, const size_t length) 
{
    size_t left = start;
    size_t right = start + length;
    size_t remaining = length;

    while (remaining >= 128) {

        uint64_t left_val = bitarray_get_u64(bitarray, left);
        uint64_t right_val = bitarray_get_u64(bitarray, right - 64);

        left_val = __builtin_bitreverse64(left_val);
        right_val = __builtin_bitreverse64(right_val);

        bitarray_set_u64(bitarray, right - 64, left_val);
        bitarray_set_u64(bitarray, left, right_val);

        left += 64;
        right -= 64;
        remaining -= 128;
    }

    size_t final_mid = left + (remaining / 2);
    for (size_t i = left; i < final_mid; ++i) {
        bool temp = bitarray_get(bitarray, i);
        size_t mirror_idx = (right - 1) - (i - left);
        
        bitarray_set(bitarray, i, bitarray_get(bitarray, mirror_idx));
        bitarray_set(bitarray, mirror_idx, temp);
    }
}


void bitarray_rotate(bitarray_t* const restrict bitarray,
                     const size_t bit_offset,
                     const size_t bit_length,
                     ssize_t bit_right_amount) 
{
    if (bit_length == 0) {
      return;
    }

    if (bit_right_amount < 0) {
        bit_right_amount = bit_length - ((-bit_right_amount) % bit_length);
    } else {
        bit_right_amount %= bit_length;
    }
    
    if (bit_right_amount == 0) {
      return;
    }
    
    const size_t split_idx = bit_length - bit_right_amount;
    bitarray_reverse_range(bitarray, bit_offset, split_idx);
    bitarray_reverse_range(bitarray, bit_offset + split_idx, bit_right_amount);
    bitarray_reverse_range(bitarray, bit_offset, bit_length);
}

inline static size_t modulo(const ssize_t n, const size_t m) {
  const ssize_t signed_m = (ssize_t)m;
  assert(signed_m > 0);
  const ssize_t result = ((n % signed_m) + signed_m) % signed_m;
  assert(result >= 0);
  return (size_t)result;
}

inline static char bitmask(const size_t bit_index) {
  return 1 << (bit_index % 8);
}

