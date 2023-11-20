/* nal_units.h
 *
 * Copyright (c) 2003-2023 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_NAL_UNITS_H
#define HANDBRAKE_NAL_UNITS_H

#include <stdint.h>

#include "handbrake/common.h"

/*
 * Write a NAL unit of the specified size to the provided
 * output buffer, using the requested output format.
 * Returns the amount (in bytes) of data written to the buffer.
 *
 * The provided NAL unit must start with the NAL unit header.
 *
 * Note: the buffer is assumed to be large enough to hold the NAL unit
 * as well as any additional data the function may prepend/append to it.
 *
 * The caller may check the minimum required buffer size by passing a
 * NULL buffer to the function and checking the returned size value.
 */
size_t hb_nal_unit_write_annexb(uint8_t *buf, const uint8_t *nal_unit, const size_t nal_unit_size);
size_t hb_nal_unit_write_isomp4(uint8_t *buf, const uint8_t *nal_unit, const size_t nal_unit_size);

/*
 * Search the provided data buffer for NAL units in Annex B format.
 *
 * Returns a pointer to the start (start code prefix excluded) of the
 * first NAL unit found, or NULL if no NAL units were found in the buffer.
 *
 * On input,  size holds the length of the provided data buffer.
 * On output, size holds the length of the returned NAL unit.
 */
uint8_t* hb_annexb_find_next_nalu(const uint8_t *start, size_t *size);

/*
 * Returns a newly-allocated buffer holding a copy of the provided
 * NAL unit bitstream data, converted to the requested format.
 */
hb_buffer_t* hb_nal_bitstream_annexb_to_mp4(const uint8_t *data, const size_t size);
hb_buffer_t* hb_nal_bitstream_mp4_to_annexb(const uint8_t *data, const size_t size, const uint8_t nal_length_size);

#endif // HANDBRAKE_NAL_UNITS_H
