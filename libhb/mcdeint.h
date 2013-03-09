/* mcdeint.h

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
struct mcdeint_private_s
{
    int              mcdeint_mode;
    int              mcdeint_qp;

    AVPacket         mcdeint_pkt;
    AVCodecContext * mcdeint_avctx_enc;
    AVFrame        * mcdeint_frame;
    AVFrame        * mcdeint_frame_dec;
};

typedef struct mcdeint_private_s mcdeint_private_t;

void mcdeint_init( mcdeint_private_t * pv,
                   int mode,
                   int qp,
                   int pix_fmt,
                   int width,
                   int height );

void mcdeint_close( mcdeint_private_t * pv );

void mcdeint_filter( hb_buffer_t * dst,
                     hb_buffer_t * src,
                     int parity,
                     mcdeint_private_t * pv );
