#include <string.h>
#include "handbrake/bitstream.h"

void bitstream_init(struct bitstream_t *bs,
                           uint8_t *buf_p, u_int32_t numBytes)
{
    bs->m_pBuf = buf_p;
    bs->m_bitPos = 0;
    bs->m_numBits = numBytes << 3;
}

void bitstream_put_bytes(struct bitstream_t *bs,
                         u_int8_t* pBytes,
                         u_int32_t numBytes)
{
    u_int32_t numBits = numBytes << 3;

    if (numBits + bs->m_bitPos > bs->m_numBits) {
        return; //throw EIO;
    }

    if ((bs->m_bitPos & 7) == 0) {
        memcpy(&bs->m_pBuf[bs->m_bitPos >> 3], pBytes, numBytes);
        bs->m_bitPos += numBits;
    } else {
        for (u_int32_t i = 0; i < numBytes; i++) {
            bitstream_put_bits(bs, pBytes[i], 8);
        }
    }
}

void bitstream_put_bits(struct bitstream_t *bs,
                        u_int32_t bits,
                        u_int32_t numBits)
{
    if (numBits + bs->m_bitPos > bs->m_numBits) {
        return; //throw EIO;
    }
    if (numBits > 32) {
        return; //throw EIO;
    }

    for (int8_t i = numBits - 1; i >= 0; i--) {
        bs->m_pBuf[bs->m_bitPos >> 3] |= ((bits >> i) & 1) << (7 - (bs->m_bitPos & 7));
        bs->m_bitPos++;
    }

}

u_int32_t bitstream_peak_bits(struct bitstream_t *bs,
                              u_int32_t numBits)
{
    if (numBits + bs->m_bitPos > bs->m_numBits) {
        return 0; //throw EIO;
    }
    if (numBits > 32) {
        return 0; //throw EIO;
    }

    u_int32_t bits = 0;
    u_int32_t bitPos = bs->m_bitPos;

    for (u_int8_t i = 0; i < numBits; i++) {
        bits <<= 1;
        bits |= (bs->m_pBuf[bitPos >> 3] >> (7 - (bitPos & 7))) & 1;
        bitPos++;
    }

    return bits;
}

u_int32_t bitstream_get_bits(struct bitstream_t *bs,
                             u_int32_t numBits)
{
    if (numBits + bs->m_bitPos > bs->m_numBits) {
        return 0; //throw EIO;
    }
    if (numBits > 32) {
        return 0; //throw EIO;
    }

    u_int32_t bits = 0;

    for (u_int8_t i = 0; i < numBits; i++) {
        bits <<= 1;
        bits |= (bs->m_pBuf[bs->m_bitPos >> 3] >> (7 - (bs->m_bitPos & 7))) & 1;
        bs->m_bitPos++;
    }

    return bits;
}

void bitstream_skip_bytes(struct bitstream_t *bs,
                          u_int32_t numBytes)
{
    bitstream_skip_bits(bs, numBytes << 3);
}

void bitstream_skip_bits(struct bitstream_t *bs,
                         u_int32_t numBits)
{
    bitstream_set_bit_position(bs, bitstream_get_bit_position(bs) + numBits);
}

u_int32_t bitstream_get_bit_position(struct bitstream_t *bs)
{
    return bs->m_bitPos;
}

void bitstream_set_bit_position(struct bitstream_t *bs,
                                u_int32_t bitPos)
{
    if (bitPos > bs->m_numBits) {
        return; //throw EIO;
    }
    bs->m_bitPos = bitPos;
}

u_int8_t* bitstream_get_buffer(struct bitstream_t *bs)
{
    return bs->m_pBuf;
}

u_int32_t bitstream_get_number_of_bytes(struct bitstream_t *bs)
{
    return (bitstream_get_number_of_bits(bs) + 7) / 8;
}

u_int32_t bitstream_get_number_of_bits(struct bitstream_t *bs)
{
    return bs->m_numBits;
}

u_int32_t bitstream_get_remaining_bits(struct bitstream_t *bs)
{
    return bs->m_numBits - bs->m_bitPos;
}
