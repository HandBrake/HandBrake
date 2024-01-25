/* nal_units.h
 *
 * Copyright (c) 2003-2024 HandBrake Team
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
 * Search the provided data buffer for NAL units.
 *
 * Returns a pointer to the start of the first NAL unit found,
 * or NULL if no NAL units were found in the buffer.
 *
 * On input,  size holds the length of the provided data buffer.
 * On output, size holds the length of the returned NAL unit.
 */
uint8_t* hb_isomp4_find_next_nalu(const uint8_t *start, size_t *size, const uint8_t nal_length_size);

/*
 * Returns a newly-allocated buffer holding a copy of the provided
 * NAL unit bitstream data, converted to the requested format.
 */
hb_buffer_t* hb_nal_bitstream_annexb_to_mp4(const uint8_t *data, const size_t size);
hb_buffer_t* hb_nal_bitstream_mp4_to_annexb(const uint8_t *data, const size_t size, const uint8_t nal_length_size);

typedef enum
{
    HB_HEVC_NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
    HB_HEVC_NAL_UNIT_CODED_SLICE_TRAIL_R,
    HB_HEVC_NAL_UNIT_CODED_SLICE_TSA_N,
    HB_HEVC_NAL_UNIT_CODED_SLICE_TSA_R,
    HB_HEVC_NAL_UNIT_CODED_SLICE_STSA_N,
    HB_HEVC_NAL_UNIT_CODED_SLICE_STSA_R,
    HB_HEVC_NAL_UNIT_CODED_SLICE_RADL_N,
    HB_HEVC_NAL_UNIT_CODED_SLICE_RADL_R,
    HB_HEVC_NAL_UNIT_CODED_SLICE_RASL_N,
    HB_HEVC_NAL_UNIT_CODED_SLICE_RASL_R,
    HB_HEVC_NAL_UNIT_CODED_SLICE_BLA_W_LP = 16,
    HB_HEVC_NAL_UNIT_CODED_SLICE_BLA_W_RADL,
    HB_HEVC_NAL_UNIT_CODED_SLICE_BLA_N_LP,
    HB_HEVC_NAL_UNIT_CODED_SLICE_IDR_W_RADL,
    HB_HEVC_NAL_UNIT_CODED_SLICE_IDR_N_LP,
    HB_HEVC_NAL_UNIT_CODED_SLICE_CRA,
    HB_HEVC_NAL_UNIT_VPS = 32,
    HB_HEVC_NAL_UNIT_SPS,
    HB_HEVC_NAL_UNIT_PPS,
    HB_HEVC_NAL_UNIT_ACCESS_UNIT_DELIMITER,
    HB_HEVC_NAL_UNIT_EOS,
    HB_HEVC_NAL_UNIT_EOB,
    HB_HEVC_NAL_UNIT_FILLER_DATA,
    HB_HEVC_NAL_UNIT_PREFIX_SEI,
    HB_HEVC_NAL_UNIT_SUFFIX_SEI,
    HB_HEVC_NAL_UNIT_UNSPECIFIED = 62,
    HB_HEVC_NAL_UNIT_INVALID = 64,
} hb_nal_type_t;

typedef enum
{
    HB_BUFFERING_PERIOD                     = 0,
    HB_PICTURE_TIMING                       = 1,
    HB_PAN_SCAN_RECT                        = 2,
    HB_FILLER_PAYLOAD                       = 3,
    HB_USER_DATA_REGISTERED_ITU_T_T35       = 4,
    HB_USER_DATA_UNREGISTERED               = 5,
    HB_RECOVERY_POINT                       = 6,
    HB_SCENE_INFO                           = 9,
    HB_FULL_FRAME_SNAPSHOT                  = 15,
    HB_PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    HB_PROGRESSIVE_REFINEMENT_SEGMENT_END   = 17,
    HB_FILM_GRAIN_CHARACTERISTICS           = 19,
    HB_POST_FILTER_HINT                     = 22,
    HB_TONE_MAPPING_INFO                    = 23,
    HB_FRAME_PACKING                        = 45,
    HB_DISPLAY_ORIENTATION                  = 47,
    HB_SOP_DESCRIPTION                      = 128,
    HB_ACTIVE_PARAMETER_SETS                = 129,
    HB_DECODING_UNIT_INFO                   = 130,
    HB_TEMPORAL_LEVEL0_INDEX                = 131,
    HB_DECODED_PICTURE_HASH                 = 132,
    HB_SCALABLE_NESTING                     = 133,
    HB_REGION_REFRESH_INFO                  = 134,
    HB_MASTERING_DISPLAY_INFO               = 137,
    HB_CONTENT_LIGHT_LEVEL_INFO             = 144,
    HB_ALTERNATIVE_TRANSFER_CHARACTERISTICS = 147,
    HB_AMBIENT_VIEWING_ENVIRONMENT          = 148,
} hb_sei_type_t;

typedef struct hb_nal_s
{
    hb_nal_type_t  type;
    size_t         payload_size;
    const uint8_t *payload;
} hb_nal_t;

typedef struct hb_sei_s
{
    hb_sei_type_t  type;
    size_t         payload_size;
    const uint8_t *payload;
} hb_sei_t;

/*
 * Returns a newly-allocated buffer holding a copy of the provided
 * NAL unit bitstream data plus the sei.
 */
hb_buffer_t * hb_isomp4_hevc_nal_bitstream_insert_payloads(const uint8_t *data,
                                                           const size_t size,
                                                           const hb_sei_t *sei,
                                                           const size_t sei_count,
                                                           const hb_nal_t *nals,
                                                           const size_t nal_count,
                                                           const uint8_t nal_length_size);

#endif // HANDBRAKE_NAL_UNITS_H
