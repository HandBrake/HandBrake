/* decssasub.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Converts SSA subtitles to either:
 * (1) TEXTSUB format: UTF-8 subtitles with limited HTML-style markup (<b>, <i>, <u>), or
 * (2) PICTURESUB format, using libass.
 *
 * SSA format references:
 *   http://www.matroska.org/technical/specs/subtitles/ssa.html
 *   http://moodub.free.fr/video/ass-specs.doc
 *   vlc-1.0.4/modules/codec/subtitles/subsass.c:ParseSSAString
 *
 * libass references:
 *   libass-0.9.9/ass.h
 *   vlc-1.0.4/modules/codec/libass.c
 *
 * @author David Foster (davidfstr)
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "hb.h"

#include <ass/ass.h>
#include "decssasub.h"
#include "colormap.h"

struct hb_work_private_s
{
    // If decoding to PICTURESUB format:
    int readOrder;

    hb_job_t *job;
};

#define SSA_2_HB_TIME(hr,min,sec,centi) \
    ( 90L * ( hr    * 1000L * 60 * 60 +\
              min   * 1000L * 60 +\
              sec   * 1000L +\
              centi * 10L ) )

#define SSA_VERBOSE_PACKETS 0

static int ssa_update_style(char *ssa, hb_subtitle_style_t *style)
{
    int pos, end, index;

    if (ssa[0] != '{')
        return 0;

    pos = 1;
    while (ssa[pos] != '}' && ssa[pos] != '\0')
    {
        index = -1;

        // Skip any malformed markup junk
        while (strchr("\\}", ssa[pos]) == NULL) pos++;
        pos++;
        // Check for an index that is in some markup (e.g. font color)
        if (isdigit(ssa[pos]))
        {
            index = ssa[pos++] - 0x30;
        }
        // Find the end of this markup clause
        end = pos;
        while (strchr("\\}", ssa[end]) == NULL) end++;
        // Handle simple integer valued attributes
        if (strchr("ibu", ssa[pos]) != NULL && isdigit(ssa[pos+1]))
        {
            int val = strtol(ssa + pos + 1, NULL, 0);
            switch (ssa[pos])
            {
                case 'i':
                    style->flags = (style->flags & ~HB_STYLE_FLAG_ITALIC) |
                                   !!val * HB_STYLE_FLAG_ITALIC;
                    break;
                case 'b':
                    style->flags = (style->flags & ~HB_STYLE_FLAG_BOLD) |
                                   !!val * HB_STYLE_FLAG_BOLD;
                    break;
                case 'u':
                    style->flags = (style->flags & ~HB_STYLE_FLAG_UNDERLINE) |
                                   !!val * HB_STYLE_FLAG_UNDERLINE;
                    break;
            }
        }
        if (ssa[pos] == 'c' && ssa[pos+1] == '&' && ssa[pos+2] == 'H')
        {
            // Font color markup
            char *endptr;
            uint32_t bgr;

            bgr = strtol(ssa + pos + 3, &endptr, 16);
            if (*endptr == '&')
            {
                switch (index)
                {
                    case -1:
                    case 1:
                        style->fg_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    case 2:
                        style->alt_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    case 3:
                        style->ol_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    case 4:
                        style->bg_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    default:
                        // Unknown color index, ignore
                        break;
                }
            }
        }
        if ((ssa[pos] == 'a' && ssa[pos+1] == '&' && ssa[pos+2] == 'H') ||
            (!strcmp(ssa+pos, "alpha") && ssa[pos+5] == '&' && ssa[pos+6] == 'H'))
        {
            // Font alpha markup
            char *endptr;
            uint8_t alpha;
            int alpha_pos = 3;

            if (ssa[1] == 'l')
                alpha_pos = 7;

            alpha = strtol(ssa + pos + alpha_pos, &endptr, 16);
            if (*endptr == '&')
            {
                // SSA alpha is inverted 0 is opaque
                alpha = 255 - alpha;
                switch (index)
                {
                    case -1:
                    case 1:
                        style->fg_alpha = alpha;
                        break;
                    case 2:
                        style->alt_alpha = alpha;
                        break;
                    case 3:
                        style->ol_alpha = alpha;
                        break;
                    case 4:
                        style->bg_alpha = alpha;
                        break;
                    default:
                        // Unknown alpha index, ignore
                        break;
                }
            }
        }
        pos = end;
    }
    if (ssa[pos] == '}')
        pos++;
    return pos;
}

char * hb_ssa_to_text(char *in, int *consumed, hb_subtitle_style_t *style)
{
    int markup_len = 0;
    int in_pos = 0;
    int out_pos = 0;
    char *out = malloc(strlen(in) + 1); // out will never be longer than in

    for (in_pos = 0; in[in_pos] != '\0'; in_pos++)
    {
        if ((markup_len = ssa_update_style(in + in_pos, style)))
        {
            *consumed = in_pos + markup_len;
            out[out_pos++] = '\0';
            return out;
        }
        // Check escape codes
        if (in[in_pos] == '\\')
        {
            in_pos++;
            switch (in[in_pos])
            {
                case '\0':
                    in_pos--;
                    break;
                case 'N':
                case 'n':
                    out[out_pos++] = '\n';
                    break;
                case 'h':
                    out[out_pos++] = ' ';
                    break;
                default:
                    out[out_pos++] = in[in_pos];
                    break;
            }
        }
        else
        {
            out[out_pos++] = in[in_pos];
        }
    }
    *consumed = in_pos;
    out[out_pos++] = '\0';
    return out;
}

void hb_ssa_style_init(hb_subtitle_style_t *style)
{
    style->flags = 0;

    style->fg_rgb    = 0x00FFFFFF;
    style->alt_rgb   = 0x00FFFFFF;
    style->ol_rgb    = 0x000F0F0F;
    style->bg_rgb    = 0x000F0F0F;

    style->fg_alpha  = 0xFF;
    style->alt_alpha = 0xFF;
    style->ol_alpha  = 0xFF;
    style->bg_alpha  = 0xFF;
}

static hb_buffer_t *ssa_decode_line_to_mkv_ssa( hb_work_object_t * w, uint8_t *in_data, int in_size, int in_sequence );

/*
 * Decodes a single SSA packet to one or more TEXTSUB or PICTURESUB subtitle packets.
 *
 * SSA packet format:
 * ( Dialogue: Marked,Start,End,Style,Name,MarginL,MarginR,MarginV,Effect,Text CR LF ) +
 *             1      2     3   4     5    6       7       8       9      10
 */
static hb_buffer_t *ssa_decode_packet( hb_work_object_t * w, hb_buffer_t *in )
{
    // Store NULL after the end of the buffer to make using string processing safe
    hb_buffer_realloc(in, ++in->size);
    in->data[in->size - 1] = '\0';

    hb_buffer_list_t list;
    hb_buffer_t *buf;

    hb_buffer_list_clear(&list);
    const char *EOL = "\r\n";
    char *curLine, *curLine_parserData;
    for ( curLine = strtok_r( (char *) in->data, EOL, &curLine_parserData );
          curLine;
          curLine = strtok_r( NULL, EOL, &curLine_parserData ) )
    {
        // Skip empty lines and spaces between adjacent CR and LF
        if (curLine[0] == '\0')
            continue;

        // Decode an individual SSA line
        buf = ssa_decode_line_to_mkv_ssa(w, (uint8_t *)curLine,
                                         strlen(curLine), in->sequence);
        hb_buffer_list_append(&list, buf);
    }

    // For point-to-point encoding, when the start time of the stream
    // may be offset, the timestamps of the subtitles must be offset as well.
    //
    // HACK: Here we are making the assumption that, under normal circumstances,
    //       the output display time of the first output packet is equal to the
    //       display time of the input packet.
    //
    //       During point-to-point encoding, the display time of the input
    //       packet will be offset to compensate.
    //
    //       Therefore we offset all of the output packets by a slip amount
    //       such that first output packet's display time aligns with the
    //       input packet's display time. This should give the correct time
    //       when point-to-point encoding is in effect.
    buf = hb_buffer_list_head(&list);
    if (buf && buf->s.start > in->s.start)
    {
        int64_t slip = buf->s.start - in->s.start;
        while (buf != NULL)
        {
            buf->s.start -= slip;
            buf->s.stop -= slip;
            buf = buf->next;
        }
    }

    return hb_buffer_list_clear(&list);
}

/*
 * Parses the start and stop time from the specified SSA packet.
 *
 * Returns true if parsing failed; false otherwise.
 */
static int parse_timing_from_ssa_packet( char *in_data, int64_t *in_start, int64_t *in_stop )
{
    /*
     * Parse Start and End fields for timing information
     */
    int start_hr, start_min, start_sec, start_centi;
    int   end_hr,   end_min,   end_sec,   end_centi;
    // SSA subtitles have an empty layer field (bare ',').  The scanf
    // format specifier "%*128[^,]" will not match on a bare ','.  There
    // must be at least one non ',' character in the match.  So the format
    // specifier is placed directly next to the ':' so that the next
    // expected ' ' after the ':' will be the character it matches on
    // when there is no layer field.
    int numPartsRead = sscanf( (char *) in_data, "Dialogue:%*128[^,],"
        "%d:%d:%d.%d,"  // Start
        "%d:%d:%d.%d,", // End
        &start_hr, &start_min, &start_sec, &start_centi,
          &end_hr,   &end_min,   &end_sec,   &end_centi );
    if ( numPartsRead != 8 )
        return 1;

    *in_start = SSA_2_HB_TIME(start_hr, start_min, start_sec, start_centi);
    *in_stop  = SSA_2_HB_TIME(  end_hr,   end_min,   end_sec,   end_centi);

    return 0;
}

static uint8_t *find_field( uint8_t *pos, uint8_t *end, int fieldNum )
{
    int curFieldID = 1;
    while (pos < end)
    {
        if ( *pos++ == ',' )
        {
            curFieldID++;
            if ( curFieldID == fieldNum )
                return pos;
        }
    }
    return NULL;
}

/*
 * SSA line format:
 *   Dialogue: Marked,Start,End,Style,Name,MarginL,MarginR,MarginV,Effect,Text '\0'
 *             1      2     3   4     5    6       7       8       9      10
 *
 * MKV-SSA packet format:
 *   ReadOrder,Marked,          Style,Name,MarginL,MarginR,MarginV,Effect,Text '\0'
 *   1         2                3     4    5       6       7       8      9
 */
static hb_buffer_t *ssa_decode_line_to_mkv_ssa( hb_work_object_t * w, uint8_t *in_data, int in_size, int in_sequence )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * out;

    // Parse values for in->s.start and in->s.stop
    int64_t in_start, in_stop;
    if ( parse_timing_from_ssa_packet( (char *) in_data, &in_start, &in_stop ) )
        goto fail;

    // Convert the SSA packet to MKV-SSA format, which is what libass expects
    char *mkvIn;
    int numPartsRead;
    char *styleToTextFields;
    char *layerField = malloc( in_size );

    // SSA subtitles have an empty layer field (bare ',').  The scanf
    // format specifier "%*128[^,]" will not match on a bare ','.  There
    // must be at least one non ',' character in the match.  So the format
    // specifier is placed directly next to the ':' so that the next
    // expected ' ' after the ':' will be the character it matches on
    // when there is no layer field.
    numPartsRead = sscanf( (char *)in_data, "Dialogue:%128[^,],", layerField );
    if ( numPartsRead != 1 )
        goto fail;

    styleToTextFields = (char *)find_field( in_data, in_data + in_size, 4 );
    if ( styleToTextFields == NULL ) {
        free( layerField );
        goto fail;
    }

    // The sscanf conversion above will result in an extra space
    // before the layerField.  Strip the space.
    char *stripLayerField = layerField;
    for(; *stripLayerField == ' '; stripLayerField++);

    out = hb_buffer_init( in_size + 1 );
    mkvIn = (char*)out->data;

    mkvIn[0] = '\0';
    sprintf(mkvIn, "%d", pv->readOrder++);    // ReadOrder: make this up
    strcat( mkvIn, "," );
    strcat( mkvIn, stripLayerField );
    strcat( mkvIn, "," );
    strcat( mkvIn, (char *)styleToTextFields );

    out->size = strlen(mkvIn) + 1;
    out->s.frametype = HB_FRAME_SUBTITLE;
    out->s.start = in_start;
    out->s.stop = in_stop;
    out->sequence = in_sequence;

    if( out->size == 0 )
    {
        hb_buffer_close(&out);
    }

    free( layerField );

    return out;

fail:
    hb_log( "decssasub: malformed SSA subtitle packet: %.*s\n", in_size, in_data );
    return NULL;
}

static int decssaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job = job;

    return 0;
}

static int decssaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;

#if SSA_VERBOSE_PACKETS
    printf("\nPACKET(%"PRId64",%"PRId64"): %.*s\n", in->s.start/90, in->s.stop/90, in->size, in->data);
#endif

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = ssa_decode_packet(w, in);

    return HB_WORK_OK;
}

static void decssaClose( hb_work_object_t * w )
{
    free( w->private_data );
}

hb_work_object_t hb_decssasub =
{
    WORK_DECSSASUB,
    "SSA Subtitle Decoder",
    decssaInit,
    decssaWork,
    decssaClose
};
