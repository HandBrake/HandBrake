/* 
   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

/*
 * Converts SSA subtitles to UTF-8 subtitles with limited HTML-style markup (<b>, <i>, <u>).
 * 
 * SSA format references:
 *   http://www.matroska.org/technical/specs/subtitles/ssa.html
 *   http://moodub.free.fr/video/ass-specs.doc
 *   vlc-1.0.4/modules/codec/subtitles/subsass.c:ParseSSAString
 * 
 * @author David Foster (davidfstr)
 */

#include <stdlib.h>
#include <stdio.h>
#include "hb.h"

typedef enum {
    BOLD        = 0x01,
    ITALIC      = 0x02,
    UNDERLINE   = 0x04
} StyleSet;

// "<b></b>".len + "<i></i>".len + "<u></u>".len
#define MAX_OVERHEAD_PER_OVERRIDE (7 * 3)

#define SSA_2_HB_TIME(hr,min,sec,centi) \
    ( 90L * ( hr    * 1000L * 60 * 60 +\
              min   * 1000L * 60 +\
              sec   * 1000L +\
              centi * 10L ) )

static StyleSet ssa_parse_style_override( char *pos, StyleSet prevStyles )
{
    StyleSet nextStyles = prevStyles;
    for (;;)
    {
        // Skip over leading '{' or last '\\'
        pos++;
        
        // Scan for next \code
        while ( *pos != '\\' && *pos != '}' && *pos != '\0' ) pos++;
        if ( *pos != '\\' )
        {
            // End of style override block
            break;
        }
        
        // If next chars are \[biu][01], interpret it
        if ( strchr("biu", pos[1]) && strchr("01", pos[2]) )
        {
            StyleSet styleID =
                pos[1] == 'b' ? BOLD :
                pos[1] == 'i' ? ITALIC :
                pos[1] == 'u' ? UNDERLINE : 0;
            int enabled = (pos[2] == '1');
            
            if (enabled)
            {
                nextStyles |= styleID;
            }
            else
            {
                nextStyles &= ~styleID;
            }
        }
    }
    return nextStyles;
}

static void ssa_append_html_tags_for_style_change(
    char **dst, StyleSet prevStyles, StyleSet nextStyles )
{
    #define APPEND(str) { \
        char *src = str; \
        while (*src) { *(*dst)++ = *src++; } \
    }

    // Reverse-order close all previous styles
    if (prevStyles & UNDERLINE) APPEND("</u>");
    if (prevStyles & ITALIC)    APPEND("</i>");
    if (prevStyles & BOLD)      APPEND("</b>");
    
    // Forward-order open all next styles
    if (nextStyles & BOLD)      APPEND("<b>");
    if (nextStyles & ITALIC)    APPEND("<i>");
    if (nextStyles & UNDERLINE) APPEND("<u>");
    
    #undef APPEND
}

/*
 * SSA packet format:
 *   Dialogue: Marked,Start,End,Style,Name,MarginL,MarginR,MarginV,Effect,Text
 *             1      2     3   4     5    6       7       8       9      10
 */
static hb_buffer_t *ssa_decode_to_utf8( hb_buffer_t *in )
{
    uint8_t *pos = in->data;
    uint8_t *end = in->data + in->size;
    
    // Store NULL after the end of the buffer to make using sscanf safe
    hb_buffer_realloc( in, in->size + 1 );
    in->data[in->size] = '\0';
    
    /*
     * Parse Start and End fields for timing information
     */
    int start_hr, start_min, start_sec, start_centi;
    int   end_hr,   end_min,   end_sec,   end_centi;
    int numPartsRead = sscanf( (char *) in->data, "%*128[^,],"
        "%d:%d:%d.%d,"  // Start
        "%d:%d:%d.%d,", // End
        &start_hr, &start_min, &start_sec, &start_centi,
          &end_hr,   &end_min,   &end_sec,   &end_centi );
    if ( numPartsRead != 8 )
        goto fail;
    
    in->start = SSA_2_HB_TIME(start_hr, start_min, start_sec, start_centi);
    in->stop  = SSA_2_HB_TIME(  end_hr,   end_min,   end_sec,   end_centi);
    
    /*
     * Advance 'pos' to the beginning of the Text field
     */
    int curFieldID = 1;
    while (pos < end)
    {
        if ( *pos++ == ',' )
        {
            curFieldID++;
            if ( curFieldID == 10 ) // Text
                break;
        }
    }
    if ( curFieldID != 10 )
        goto fail;
    uint8_t *textFieldPos = pos;
    
    // Count the number of style overrides in the Text field
    int numStyleOverrides = 0;
    while ( pos < end )
    {
        if (*pos++ == '{')
        {
            numStyleOverrides++;
        }
    }
    
    int maxOutputSize = (end - pos) + ((numStyleOverrides + 1) * MAX_OVERHEAD_PER_OVERRIDE);
    hb_buffer_t *out = hb_buffer_init( maxOutputSize );
    if ( out == NULL )
        return NULL;
    
    /*
     * The Text field contains plain text marked up with:
     * (1) '\n' -> space
     * (2) '\N' -> newline
     * (3) curly-brace control codes like '{\k44}' -> empty (strip them)
     * 
     * Perform the above conversions and copy it to the output packet
     */
    StyleSet prevStyles = 0;
    uint8_t *dst = out->data;
    pos = textFieldPos;
    while ( pos < end )
    {
        if ( pos[0] == '\\' && pos[1] == 'n' )
        {
            *dst++ = ' ';
            pos += 2;
        }
        else if ( pos[0] == '\\' && pos[1] == 'N' )
        {
            *dst++ = '\n';
            pos += 2;
        }
        else if ( pos[0] == '{' )
        {
            // Parse SSA style overrides and append appropriate HTML style tags
            StyleSet nextStyles = ssa_parse_style_override( pos, prevStyles );
            ssa_append_html_tags_for_style_change( &dst, prevStyles, nextStyles );
            prevStyles = nextStyles;
            
            // Skip past SSA control code
            while ( pos < end && *pos != '}' ) pos++;
            if    ( pos < end && *pos == '}' ) pos++;
        }
        else
        {
            // Copy raw character
            *dst++ = *pos++;
        }
    }
    
    // Append closing HTML style tags
    ssa_append_html_tags_for_style_change( &dst, prevStyles, 0 );
    
    // Trim output buffer to the actual amount of data written
    out->size = dst - out->data;
    
    // Copy metadata from the input packet to the output packet
    out->start = in->start;
    out->stop = in->stop;
    
    return out;
    
fail:
    hb_log( "decssasub: malformed SSA subtitle packet: %.*s\n", in->size, in->data );
    return NULL;
}

static int decssaInit( hb_work_object_t * w, hb_job_t * job )
{
    return 0;
}

static int decssaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = NULL;
    
    if ( in->size > 0 ) {
        out = ssa_decode_to_utf8(in);
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

static void decssaClose( hb_work_object_t * w )
{
    // nothing
}

hb_work_object_t hb_decssasub =
{
    WORK_DECSSASUB,
    "SSA Subtitle Decoder",
    decssaInit,
    decssaWork,
    decssaClose
};
