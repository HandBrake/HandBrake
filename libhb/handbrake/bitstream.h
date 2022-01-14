
#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>

struct bitstream_t {
    u_int8_t*    m_pBuf;
    u_int32_t    m_bitPos;
    u_int32_t    m_numBits;
};

void bitstream_init(struct bitstream_t *bs,
                    uint8_t *buf_p, u_int32_t numBytes);

void bitstream_put_bytes(struct bitstream_t *bs,
                         u_int8_t* pBytes,
                         u_int32_t numBytes);

void bitstream_put_bits(struct bitstream_t *bs,
                        u_int32_t bits,
                        u_int32_t numBits);

u_int32_t bitstream_peak_bits(struct bitstream_t *bs,
                              u_int32_t numBits);
u_int32_t bitstream_get_bits(struct bitstream_t *bs,
                             u_int32_t numBits);

void bitstream_skip_bytes(struct bitstream_t *bs,
                          u_int32_t numBytes);

void bitstream_skip_bits(struct bitstream_t *bs,
                         u_int32_t numBits);

u_int32_t bitstream_get_bit_position(struct bitstream_t *bs);

void bitstream_set_bit_position(struct bitstream_t *bs,
                                u_int32_t bitPos);

u_int8_t* bitstream_get_buffer(struct bitstream_t *bs);

u_int32_t bitstream_get_number_of_bytes(struct bitstream_t *bs);

u_int32_t bitstream_get_number_of_bits(struct bitstream_t *bs);

u_int32_t bitstream_get_remaining_bits(struct bitstream_t *bs);

#endif
