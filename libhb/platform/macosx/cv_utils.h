/* cv_utils.h

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_COREVIDEO_UTILS_H
#define HB_COREVIDEO_UTILS_H

#include <CoreVideo/CoreVideo.h>
#include <CoreMedia/CoreMedia.h>

OSType hb_cv_get_pixel_format(enum AVPixelFormat pix_fmt,
                              enum AVColorRange color_range);

CVPixelBufferRef hb_cv_get_pixel_buffer(const hb_buffer_t *buf);

int hb_cv_get_io_surface_usage_count(const hb_buffer_t *buf);


CVPixelBufferPoolRef hb_cv_create_pixel_buffer_pool(int width, int height,
                                                    enum AVPixelFormat pix_fmt,
                                                    enum AVColorRange color_range);

CFStringRef hb_cv_colr_pri_xlat(int color_prim);
CFStringRef hb_cv_colr_tra_xlat(int color_transfer);
CFNumberRef hb_cv_colr_gamma_xlat(int color_transfer) CF_RETURNS_RETAINED;
CFStringRef hb_cv_colr_mat_xlat(int color_matrix);
CFStringRef hb_cv_colr_range_xlat(int color_range);
CFStringRef hb_cv_chroma_loc_xlat(int chroma_location);

void hb_cv_add_color_tag(CFMutableDictionaryRef attachments,
                         int color_prim, int color_transfer,
                         int color_matrix, int chroma_location);

void hb_cv_set_attachments(CVPixelBufferRef pix_buf, CFDictionaryRef attachments);

int hb_cv_match_rgb_to_colorspace(int rgb,
                                  int color_prim,
                                  int color_transfer,
                                  int color_matrix);

#endif /* HB_COREVIDEO_UTILS_H */
