/* dectx3gsub.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Converts TX3G subtitles to UTF-8 subtitles with limited HTML-style markup (<b>, <i>, <u>).
 *
 * TX3G == MPEG 4, Part 17 (ISO/IEC 14496-17) == 3GPP Timed Text (26.245)
 * A full reference to the format can be found here:
 * http://www.3gpp.org/ftp/Specs/html-info/26245.htm
 *
 * @author David Foster (davidfstr)
 */

#include <stdlib.h>
#include <stdio.h>
#include "handbrake/handbrake.h"
#include "handbrake/colormap.h"

struct hb_work_private_s
{
    int line;   // SSA line number
};

typedef enum {
    BOLD        = 0x1,
    ITALIC      = 0x2,
    UNDERLINE   = 0x4
} FaceStyleFlag;

#define MAX_MARKUP_LEN 40
#define SSA_PREAMBLE_LEN 24

typedef struct {
    uint16_t startChar;       // NOTE: indices in terms of *character* (not: byte) positions
    uint16_t endChar;
    uint16_t fontID;
    uint8_t faceStyleFlags;   // FaceStyleFlag
    uint8_t fontSize;
    uint32_t textColorRGBA;
} StyleRecord;

// NOTE: None of these macros check for buffer overflow
#define READ_U8()       *pos;                                                                 pos += 1;
#define READ_U16()      (pos[0] << 8) | pos[1];                                               pos += 2;
#define READ_U32()      ((uint32_t)pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];   pos += 4;
#define READ_ARRAY(n)   pos;                                                                  pos += n;
#define SKIP_ARRAY(n)   pos += n;

#define WRITE_CHAR(c)       {dst[0]=c;                                              dst += 1;}

#define FOURCC(str)    ((((uint32_t) str[0]) << 24) | \
                        (((uint32_t) str[1]) << 16) | \
                        (((uint32_t) str[2]) << 8) | \
                        (((uint32_t) str[3]) << 0))
#define IS_10xxxxxx(c) ((c & 0xC0) == 0x80)

static int write_ssa_markup(char *dst, StyleRecord *style)
{
    if (style == NULL)
    {
        sprintf(dst, "{\\r}");
        return strlen(dst);
    }
    sprintf(dst, "{\\i%d\\b%d\\u%d\\1c&H%X&\\1a&H%02X&}",
        !!(style->faceStyleFlags & ITALIC),
        !!(style->faceStyleFlags & BOLD),
        !!(style->faceStyleFlags & UNDERLINE),
        HB_RGB_TO_BGR(style->textColorRGBA >> 8),
        255 - (style->textColorRGBA & 0xFF)); // SSA alpha is inverted 0==opaque

    return strlen(dst);
}

static hb_buffer_t *tx3g_decode_to_ssa(hb_work_private_t *pv, hb_buffer_t *in)
{
    hb_buffer_t *out = NULL;
    uint8_t *pos = in->data;
    uint8_t *end = in->data + in->size;

    uint16_t numStyleRecords = 0;
    StyleRecord *styleRecords = NULL;

    /*
     * Parse the packet as a TX3G TextSample.
     *
     * Look for a single StyleBox ('styl') and read all contained StyleRecords.
     * Ignore all other box types.
     *
     * NOTE: Buffer overflows on read are not checked.
     */
    uint16_t textLength = READ_U16();
    uint8_t *text = READ_ARRAY(textLength);
    while ( pos < end )
    {
        /*
         * Read TextSampleModifierBox
         */
        uint32_t size = READ_U32();
        if ( size == 0 )
        {
            size = pos - end;   // extends to end of packet
        }
        if ( size == 1 )
        {
            hb_log( "dectx3gsub: TextSampleModifierBox has unsupported large size" );
            break;
        }
        uint32_t type = READ_U32();
        if (type == FOURCC("uuid"))
        {
            hb_log( "dectx3gsub: TextSampleModifierBox has unsupported extended type" );
            break;
        }

        if (type == FOURCC("styl"))
        {
            // Found a StyleBox. Parse the contained StyleRecords

            if ( numStyleRecords != 0 )
            {
                hb_log( "dectx3gsub: found additional StyleBoxes on subtitle; skipping" );
                SKIP_ARRAY(size);
                continue;
            }

            numStyleRecords = READ_U16();
            if (numStyleRecords > 0)
            {
                styleRecords = calloc(numStyleRecords, sizeof(StyleRecord));
                if (styleRecords == NULL)
                {
                    goto fail;
                }
            }

            int i;
            for (i = 0; i < numStyleRecords; i++)
            {
                styleRecords[i].startChar         = READ_U16();
                styleRecords[i].endChar           = READ_U16();
                styleRecords[i].fontID            = READ_U16();
                styleRecords[i].faceStyleFlags    = READ_U8();
                styleRecords[i].fontSize          = READ_U8();
                styleRecords[i].textColorRGBA     = READ_U32();
            }
        }
        else
        {
            // Found some other kind of TextSampleModifierBox. Skip it.
            SKIP_ARRAY(size);
        }
    }

    /*
     * Copy text to output buffer, and add HTML markup for the style records
     */
    int maxOutputSize = textLength + SSA_PREAMBLE_LEN + (numStyleRecords * MAX_MARKUP_LEN);
    out = hb_buffer_init( maxOutputSize );
    if ( out == NULL )
        goto fail;
    uint8_t *dst = out->data;
    uint8_t *start;
    int charIndex = 0;
    int styleIndex = 0;

    snprintf((char*)dst, maxOutputSize, "%d,0,Default,,0,0,0,,", pv->line);
    dst += strlen((char*)dst);
    start = dst;
    for (pos = text, end = text + textLength; pos < end; pos++)
    {
        if (IS_10xxxxxx(*pos))
        {
            // Is a non-first byte of a multi-byte UTF-8 character
            WRITE_CHAR(*pos);
            continue;   // ...without incrementing 'charIndex'
        }

        if (styleIndex < numStyleRecords)
        {
            if (styleRecords[styleIndex].endChar == charIndex)
            {
                if (styleIndex + 1 >= numStyleRecords ||
                    styleRecords[styleIndex+1].startChar > charIndex)
                {
                    dst += write_ssa_markup((char*)dst, NULL);
                }
                styleIndex++;
            }
            if (styleIndex < numStyleRecords && styleRecords[styleIndex].startChar == charIndex)
            {
                dst += write_ssa_markup((char*)dst, &styleRecords[styleIndex]);
            }
        }

        if (*pos == '\n')
        {
            WRITE_CHAR('\\');
            WRITE_CHAR('N');
        }
        else
        {
            WRITE_CHAR(*pos);
        }
        charIndex++;
    }
    if (start == dst)
    {
        // No text in the subtitle.  This sub is just filler, drop it.
        free(styleRecords);
        hb_buffer_close(&out);
        return NULL;
    }
    *dst = '\0';
    dst++;

    pv->line++;

    // Trim output buffer to the actual amount of data written
    out->size = dst - out->data;

    // Copy metadata from the input packet to the output packet
    out->s.frametype    = HB_FRAME_SUBTITLE;
    out->s.start        = in->s.start;
    out->s.stop         = in->s.stop;
    out->s.scr_sequence = in->s.scr_sequence;

fail:
    free(styleRecords);

    return out;
}

#undef READ_U8
#undef READ_U16
#undef READ_U32
#undef READ_ARRAY
#undef SKIP_ARRAY

#undef WRITE_CHAR
#undef WRITE_START_TAG
#undef WRITE_END_TAG

static int dectx3gInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;
    pv = calloc( 1, sizeof( hb_work_private_t ) );
    if (pv == NULL)
        return 1;
    w->private_data = pv;

    // TODO:
    // parse w->subtitle->extradata txg3 sample description into
    // SSA format and replace extradata.
    // For now we just create a generic SSA Script Info.
    int height = job->title->geometry.height - job->crop[0] - job->crop[1];
    int width = job->title->geometry.width - job->crop[2] - job->crop[3];
    hb_subtitle_add_ssa_header(w->subtitle, HB_FONT_SANS,
                               .066 * job->title->geometry.height,
                               width, height);

    return 0;
}

static int dectx3gWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if ( in->s.stop == 0 ) {
        hb_log( "dectx3gsub: subtitle packet lacks duration" );
    }

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = tx3g_decode_to_ssa(pv, in);

    return HB_WORK_OK;
}

static void dectx3gClose( hb_work_object_t * w )
{
    free(w->private_data);
}

hb_work_object_t hb_dectx3gsub =
{
    WORK_DECTX3GSUB,
    "TX3G Subtitle Decoder",
    dectx3gInit,
    dectx3gWork,
    dectx3gClose
};
