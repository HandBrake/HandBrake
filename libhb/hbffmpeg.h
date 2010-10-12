/* This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

void hb_avcodec_init(void);
int hb_avcodec_open( AVCodecContext *, struct AVCodec * );
int hb_avcodec_close( AVCodecContext * );
int hb_av_find_stream_info(AVFormatContext *ic);
int hb_ff_layout_xlat(int64_t ff_layout, int channels);
struct SwsContext*
hb_sws_get_context(int srcW, int srcH, enum PixelFormat srcFormat,
                   int dstW, int dstH, enum PixelFormat dstFormat,
                   int flags);
