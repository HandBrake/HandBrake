
#ifndef DYNAMIC_HDR_METADATA_H
#define DYNAMIC_HDR_METADATA_H

#include <stdint.h>
#include "libavutil/hdr_dynamic_metadata.h"

void hb_hdr_10_sidedata_to_sei(const AVFrameSideData *side_data, uint8_t **buf_p, uint32_t *numBytes);

#endif
