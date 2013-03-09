/* dectx3gsub.c

   Copyright (c) 2003-2013 HandBrake Team
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
#include "hb.h"

typedef enum {
    BOLD        = 0x1,
    ITALIC      = 0x2,
    UNDERLINE   = 0x4
} FaceStyleFlag;

#define NUM_FACE_STYLE_FLAGS 3
#define MAX_OPEN_TAG_SIZE 3     // "<b>"
#define MAX_CLOSE_TAG_SIZE 4    // "</b>"

typedef struct {
    uint16_t startChar;       // NOTE: indices in terms of *character* (not: byte) positions
    uint16_t endChar;
    uint16_t fontID;
    uint8_t faceStyleFlags;   // FaceStyleFlag
    uint8_t fontSize;
    uint32_t textColorRGBA;
} StyleRecord;

// NOTE: None of these macros check for buffer overflow
#define READ_U8()       *pos;                                                       pos += 1;
#define READ_U16()      (pos[0] << 8) | pos[1];                                     pos += 2;
#define READ_U32()      (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];   pos += 4;
#define READ_ARRAY(n)   pos;                                                        pos += n;
#define SKIP_ARRAY(n)   pos += n;

#define WRITE_CHAR(c)       {dst[0]=c;                                              dst += 1;}
#define WRITE_START_TAG(c)  {dst[0]='<'; dst[1]=c;   dst[2]='>';                    dst += 3;}
#define WRITE_END_TAG(c)    {dst[0]='<'; dst[1]='/'; dst[2]=c; dst[3]='>';          dst += 4;}

#define FOURCC(str)    ((((uint32_t) str[0]) << 24) | \
                        (((uint32_t) str[1]) << 16) | \
                        (((uint32_t) str[2]) << 8) | \
                        (((uint32_t) str[3]) << 0))
#define IS_10xxxxxx(c) ((c & 0xC0) == 0x80)

static hb_buffer_t *tx3g_decode_to_utf8( hb_buffer_t *in )
{
    uint8_t *pos = in->data;
    uint8_t *end = in->data + in->size;
    
    uint16_t numStyleRecords = 0;
    
    uint8_t *startStyle;
    uint8_t *endStyle;
    
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
    startStyle = calloc( textLength, 1 );
    endStyle = calloc( textLength, 1 );
    while ( pos < end ) {
        /*
         * Read TextSampleModifierBox
         */
        uint32_t size = READ_U32();
        if ( size == 0 ) {
            size = pos - end;   // extends to end of packet
        }
        if ( size == 1 ) {
            hb_log( "dectx3gsub: TextSampleModifierBox has unsupported large size" );
            break;
        }
        uint32_t type = READ_U32();
        if ( type == FOURCC("uuid") ) {
            hb_log( "dectx3gsub: TextSampleModifierBox has unsupported extended type" );
            break;
        }
        
        if ( type == FOURCC("styl") ) {
            // Found a StyleBox. Parse the contained StyleRecords
            
            if ( numStyleRecords != 0 ) {
                hb_log( "dectx3gsub: found additional StyleBoxes on subtitle; skipping" );
                SKIP_ARRAY(size);
                continue;
            }
            
            numStyleRecords = READ_U16();
            
            int i;
            for (i=0; i<numStyleRecords; i++) {
                StyleRecord curRecord;
                curRecord.startChar         = READ_U16();
                curRecord.endChar           = READ_U16();
                curRecord.fontID            = READ_U16();
                curRecord.faceStyleFlags    = READ_U8();
                curRecord.fontSize          = READ_U8();
                curRecord.textColorRGBA     = READ_U32();
                
                startStyle[curRecord.startChar] |= curRecord.faceStyleFlags;
                endStyle[curRecord.endChar]     |= curRecord.faceStyleFlags;
            }
        } else {
            // Found some other kind of TextSampleModifierBox. Skip it.
            SKIP_ARRAY(size);
        }
    }
    
    /*
     * Copy text to output buffer, and add HTML markup for the style records
     */
    int maxOutputSize = textLength + (numStyleRecords * NUM_FACE_STYLE_FLAGS * (MAX_OPEN_TAG_SIZE + MAX_CLOSE_TAG_SIZE));
    hb_buffer_t *out = hb_buffer_init( maxOutputSize );
    if ( out == NULL )
        goto fail;
    uint8_t *dst = out->data;
    int charIndex = 0;
    for ( pos = text, end = text + textLength; pos < end; pos++ ) {
        if (IS_10xxxxxx(*pos)) {
            // Is a non-first byte of a multi-byte UTF-8 character
            WRITE_CHAR(*pos);
            continue;   // ...without incrementing 'charIndex'
        }
        
        uint8_t plusStyles = startStyle[charIndex];
        uint8_t minusStyles = endStyle[charIndex];
        
        if (minusStyles & UNDERLINE)
            WRITE_END_TAG('u');
        if (minusStyles & ITALIC)
            WRITE_END_TAG('i');
        if (minusStyles & BOLD)
            WRITE_END_TAG('b');
        
        if (plusStyles & BOLD)
            WRITE_START_TAG('b');
        if (plusStyles & ITALIC)
            WRITE_START_TAG('i');
        if (plusStyles & UNDERLINE)
            WRITE_START_TAG('u');
        
        WRITE_CHAR(*pos);
        charIndex++;
    }
    
    // Trim output buffer to the actual amount of data written
    out->size = dst - out->data;
    
    // Copy metadata from the input packet to the output packet
    out->s.start = in->s.start;
    out->s.stop = in->s.stop;
    
fail:
    free( startStyle );
    free( endStyle );
    
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
    return 0;
}

static int dectx3gWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = NULL;
    
    // Warn if the subtitle's duration has not been passed through by the demuxer,
    // which will prevent the subtitle from displaying at all
    if ( in->s.stop == 0 ) {
        hb_log( "dectx3gsub: subtitle packet lacks duration" );
    }
    
    if ( in->size > 0 ) {
        out = tx3g_decode_to_utf8(in);
    } else {
        out = hb_buffer_init( 0 );
    }
    
    if ( out != NULL ) {
        // We shouldn't be storing the extra NULL character,
        // but the MP4 muxer expects this, unfortunately.
        if ( out->size > 0 && out->data[out->size - 1] != '\0' ) {
            // NOTE: out->size remains unchanged
            hb_buffer_realloc( out, out->size + 1 );
            out->data[out->size] = '\0';
        }
        
        // If the input packet was non-empty, do not pass through
        // an empty output packet (even if the subtitle was empty),
        // as this would be interpreted as an end-of-stream
        if ( in->size > 0 && out->size == 0 ) {
            hb_buffer_close(&out);
        }
    }
    
    // Dispose the input packet, as it is no longer needed
    hb_buffer_close(&in);
    
    *buf_in = NULL;
    *buf_out = out;
    return HB_WORK_OK;
}

static void dectx3gClose( hb_work_object_t * w )
{
    // nothing
}

hb_work_object_t hb_dectx3gsub =
{
    WORK_DECTX3GSUB,
    "TX3G Subtitle Decoder",
    dectx3gInit,
    dectx3gWork,
    dectx3gClose
};
