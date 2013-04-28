/* decssasub.c

   Copyright (c) 2003-2013 HandBrake Team
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
#include "hb.h"

#include <ass/ass.h>

struct hb_work_private_s
{
    // If decoding to PICTURESUB format:
    int readOrder;

    hb_job_t *job;
};

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

#define SSA_VERBOSE_PACKETS 0

static StyleSet ssa_parse_style_override( uint8_t *pos, StyleSet prevStyles )
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
    uint8_t **dst, StyleSet prevStyles, StyleSet nextStyles )
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

static hb_buffer_t *ssa_decode_line_to_utf8( uint8_t *in_data, int in_size, int in_sequence );
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
    
    hb_buffer_t *out_list = NULL;
    hb_buffer_t **nextPtr = &out_list;
    
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
        hb_buffer_t *out;
        if ( w->subtitle->config.dest == PASSTHRUSUB ) {
            out = ssa_decode_line_to_utf8( (uint8_t *) curLine, strlen( curLine ), in->sequence );
            if ( out == NULL )
                continue;
            
            // We shouldn't be storing the extra NULL character,
            // but the MP4 muxer expects this, unfortunately.
            if (out->size > 0 && out->data[out->size - 1] != '\0')
            {
                hb_buffer_realloc(out, ++out->size);
                out->data[out->size - 1] = '\0';
            }
            
            // If the input packet was non-empty, do not pass through
            // an empty output packet (even if the subtitle was empty),
            // as this would be interpreted as an end-of-stream
            if ( in->size > 0 && out->size == 0 ) {
                hb_buffer_close(&out);
                continue;
            }
        } else if ( w->subtitle->config.dest == RENDERSUB ) {
            out = ssa_decode_line_to_mkv_ssa( w, (uint8_t *) curLine, strlen( curLine ), in->sequence );
            if ( out == NULL )
                continue;
        }
        
        // Append 'out' to 'out_list'
        *nextPtr = out;
        nextPtr = &out->next;
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
    if (out_list && out_list->s.start > in->s.start)
    {
        int64_t slip = out_list->s.start - in->s.start;
        hb_buffer_t *out;

        out = out_list;
        while (out)
        {
            out->s.start -= slip;
            out->s.stop -= slip;
            out = out->next;
        }
    }
    
    return out_list;
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
 */
static hb_buffer_t *ssa_decode_line_to_utf8( uint8_t *in_data, int in_size, int in_sequence )
{
    uint8_t *pos = in_data;
    uint8_t *end = in_data + in_size;
    
    // Parse values for in->s.start and in->s.stop
    int64_t in_start, in_stop;
    if ( parse_timing_from_ssa_packet( (char *) in_data, &in_start, &in_stop ) )
        goto fail;
    
    uint8_t *textFieldPos = find_field( pos, end, 10 );
    if ( textFieldPos == NULL )
        goto fail;
    
    // Count the number of style overrides in the Text field
    int numStyleOverrides = 0;
    pos = textFieldPos;
    while ( pos < end )
    {
        if (*pos++ == '{')
        {
            numStyleOverrides++;
        }
    }
    
    int maxOutputSize = (end - textFieldPos) + ((numStyleOverrides + 1) * MAX_OVERHEAD_PER_OVERRIDE);
    hb_buffer_t *out = hb_buffer_init( maxOutputSize );
    if ( out == NULL )
        return NULL;
    
    /*
     * The Text field contains plain text marked up with:
     * (1) '\n' -> space
     * (2) '\N' -> newline
     * (3) curly-brace control codes like '{\k44}' -> HTML tags / strip
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
    out->s.start = in_start;
    out->s.stop = in_stop;
    out->sequence = in_sequence;
    
    return out;
    
fail:
    hb_log( "decssasub: malformed SSA subtitle packet: %.*s\n", in_size, in_data );
    return NULL;
}

static hb_buffer_t * ssa_to_mkv_ssa( hb_work_object_t * w,  hb_buffer_t * in )
{
    hb_buffer_t * out_last = NULL;
    hb_buffer_t * out_first = NULL;

    // Store NULL after the end of the buffer to make using string processing safe
    hb_buffer_realloc(in, ++in->size);
    in->data[in->size - 1] = '\0';

    const char *EOL = "\r\n";
    char *curLine, *curLine_parserData;
    for ( curLine = strtok_r( (char *) in->data, EOL, &curLine_parserData );
          curLine;
          curLine = strtok_r( NULL, EOL, &curLine_parserData ) )
    {
        hb_buffer_t * out;

        out = ssa_decode_line_to_mkv_ssa( w, (uint8_t *) curLine, strlen( curLine ), in->sequence );
        if( out )
        {
            if ( out_last == NULL )
            {
                out_last = out_first = out;
            }
            else
            {
                out_last->next = out;
                out_last = out;
            }
        }
    }

    return out_first;
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
    strcat( mkvIn, (char *) styleToTextFields );
    
    out->size = strlen(mkvIn);
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
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;
    
#if SSA_VERBOSE_PACKETS
    printf("\nPACKET(%"PRId64",%"PRId64"): %.*s\n", in->s.start/90, in->s.stop/90, in->size, in->data);
#endif
    
    if ( in->size <= 0 )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    if ( w->subtitle->config.dest == PASSTHRUSUB && pv->job->mux == HB_MUX_MKV )
    {
        *buf_out = ssa_to_mkv_ssa(w, in);
    }
    else
    {
        *buf_out = ssa_decode_packet(w, in);
    }

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
