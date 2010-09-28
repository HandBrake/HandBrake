/* 
   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

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
    ASS_Library *ssa;
    ASS_Renderer *renderer;
    ASS_Track *ssaTrack;
    int readOrder;
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
static hb_buffer_t *ssa_decode_line_to_picture( hb_work_object_t * w, uint8_t *in_data, int in_size, int in_sequence );

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
    hb_buffer_realloc( in, in->size + 1 );
    in->data[in->size] = '\0';
    
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
                continue;
            }
        } else if ( w->subtitle->config.dest == RENDERSUB ) {
            out = ssa_decode_line_to_picture( w, (uint8_t *) curLine, strlen( curLine ), in->sequence );
            if ( out == NULL )
                continue;
        }
        
        // Append 'out' to 'out_list'
        *nextPtr = out;
        nextPtr = &out->next;
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
    int numPartsRead = sscanf( (char *) in_data, "Dialogue: %*128[^,],"
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
    
    // Parse values for in->start and in->stop
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
    out->start = in_start;
    out->stop = in_stop;
    out->sequence = in_sequence;
    
    return out;
    
fail:
    hb_log( "decssasub: malformed SSA subtitle packet: %.*s\n", in_size, in_data );
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
static hb_buffer_t *ssa_decode_line_to_picture( hb_work_object_t * w, uint8_t *in_data, int in_size, int in_sequence )
{
    hb_work_private_t * pv = w->private_data;
    
    // Parse values for in->start and in->stop
    int64_t in_start, in_stop;
    if ( parse_timing_from_ssa_packet( (char *) in_data, &in_start, &in_stop ) )
        goto fail;
    
    // Convert the SSA packet to MKV-SSA format, which is what libass expects
    char *mkvIn;
    int mkvInSize;
    {
        char *layerField = malloc( in_size );
        int numPartsRead = sscanf( (char *) in_data, "Dialogue: %128[^,],", layerField );
        if ( numPartsRead != 1 )
            goto fail;
        
        char *styleToTextFields = (char *) find_field( in_data, in_data + in_size, 4 );
        if ( styleToTextFields == NULL ) {
            free( layerField );
            goto fail;
        }
        
        mkvIn = malloc( in_size + 1 );
        mkvIn[0] = '\0';
        sprintf(mkvIn, "%d", pv->readOrder++);    // ReadOrder: make this up
        strcat( mkvIn, "," );
        strcat( mkvIn, layerField );
        strcat( mkvIn, "," );
        strcat( mkvIn, (char *) styleToTextFields );
        
        mkvInSize = strlen(mkvIn);
        
        free( layerField );
    }
    
    // Parse MKV-SSA packet
    ass_process_chunk( pv->ssaTrack, mkvIn, mkvInSize, in_start / 90, (in_stop - in_start) / 90 );
    
    free( mkvIn );
    
    // TODO: To support things like karaoke, it won't be sufficient to only generate
    //       new subtitle pictures when there are subtitle packets. Rather, pictures will
    //       need to be generated potentially continuously. 
    //       
    //       Until "karaoke support" is implemented, make an educated guess about the
    //       timepoint within the subtitle that should be rendered. I guess the midpoint.
    int64_t renderTime = ( in_start + in_stop ) / 2; 
    
    int changed;
    ASS_Image *frameList = ass_render_frame( pv->renderer, pv->ssaTrack, renderTime / 90, &changed );
    if ( !changed && !frameList )
        return NULL;
    
    int numFrames = 0;
    ASS_Image *curFrame;
    for (curFrame = frameList; curFrame; curFrame = curFrame->next)
        numFrames++;
    
    hb_buffer_t *outSubpictureList = NULL;
    hb_buffer_t **outSubpictureListTailPtr = &outSubpictureList;
    
    // Generate a PICTURESUB packet from the frames
    ASS_Image *frame;
    for (frame = frameList; frame; frame = frame->next) {
        // Allocate pixmap where drawing will be done
        uint8_t *rgba = calloc(frame->w * frame->h * 4, 1);
        
        unsigned r = (frame->color >> 24) & 0xff;
        unsigned g = (frame->color >> 16) & 0xff;
        unsigned b = (frame->color >>  8) & 0xff;
        unsigned a = (frame->color      ) & 0xff;
        
        int x, y;
        for (y = 0; y < frame->h; y++) {
            for (x = 0; x < frame->w; x++) {
                unsigned srcAlphaPrenormalized = frame->bitmap[y*frame->stride + x];
                unsigned srcAlpha = (255 - a) * srcAlphaPrenormalized / 255;
                
                uint8_t *dst = &rgba[(y*frame->w + x) * 4];
                unsigned oldDstAlpha = dst[3];
                
                if (oldDstAlpha == 0) {
                    // Optimized version
                    dst[0] = r;
                    dst[1] = g;
                    dst[2] = b;
                    dst[3] = srcAlpha;
                } else {
                    dst[3] = 255 - ( 255 - dst[3] ) * ( 255 - srcAlpha ) / 255;
                    if (dst[3] != 0) {
                        dst[0] = ( dst[0] * oldDstAlpha * (255-srcAlpha) / 255 + r * srcAlpha ) / dst[3];
                        dst[1] = ( dst[1] * oldDstAlpha * (255-srcAlpha) / 255 + g * srcAlpha ) / dst[3];
                        dst[2] = ( dst[2] * oldDstAlpha * (255-srcAlpha) / 255 + b * srcAlpha ) / dst[3];
                    }
                }
            }
        }
        
        // Generate output subpicture (in PICTURESUB format)
        hb_buffer_t *out = hb_buffer_init(frame->w * frame->h * 4);
        out->x = frame->dst_x;
        out->y = frame->dst_y;
        out->width = frame->w;
        out->height = frame->h;
        
        int i;
        int numPixels = frame->w * frame->h;
        for (i = 0; i < numPixels; i++) {
            uint8_t *srcRgba = &rgba[i * 4];
            
            uint8_t *dstY = &out->data[(numPixels * 0) + i];
            uint8_t *dstA = &out->data[(numPixels * 1) + i];
            uint8_t *dstU = &out->data[(numPixels * 2) + i];
            uint8_t *dstV = &out->data[(numPixels * 3) + i];
            
            int srcYuv = hb_rgb2yuv((srcRgba[0] << 16) | (srcRgba[1] << 8) | (srcRgba[2] << 0));
            int srcA = srcRgba[3];
            
            *dstY = (srcYuv >> 16) & 0xff;
            *dstU = (srcYuv >> 8 ) & 0xff;
            *dstV = (srcYuv >> 0 ) & 0xff;
            *dstA = srcA / 16;  // HB's max alpha value is 16
        }
        
        free(rgba);
        
        *outSubpictureListTailPtr = out;
        outSubpictureListTailPtr = &out->next_subpicture;
    }
    
    // NOTE: The subpicture list is actually considered a single packet by most other code
    hb_buffer_t *out = outSubpictureList;
    
    // Copy metadata from the input packet to the output packet
    out->start = in_start;
    out->stop = in_stop;
    out->sequence = in_sequence;

    return out;
    
fail:
    hb_log( "decssasub: malformed SSA subtitle packet: %.*s\n", in_size, in_data );
    return NULL;
}

static void ssa_log(int level, const char *fmt, va_list args, void *data)
{
    if ( level < 5 )      // same as default verbosity when no callback is set
    {
        char *msg;
        if ( vasprintf( &msg, fmt, args ) < 0 )
        {
            hb_log( "decssasub: could not report libass message\n" );
            return;
        }
        hb_log( "[ass] %s", msg );  // no need for extra '\n' because libass sends it
        
        free( msg );
    }
}

static int decssaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    
    if ( w->subtitle->config.dest == RENDERSUB ) {
        pv->ssa = ass_library_init();
        if ( !pv->ssa ) {
            hb_log( "decssasub: libass initialization failed\n" );
            return 1;
        }
        
        // Redirect libass output to hb_log
        ass_set_message_cb( pv->ssa, ssa_log, NULL );
        
        // Load embedded fonts
        hb_list_t * list_attachment = job->title->list_attachment;
        int i;
        for ( i = 0; i < hb_list_count(list_attachment); i++ )
        {
            hb_attachment_t * attachment = hb_list_item( list_attachment, i );
            
            if ( attachment->type == FONT_TTF_ATTACH )
            {
                ass_add_font(
                    pv->ssa,
                    attachment->name,
                    attachment->data,
                    attachment->size );
            }
        }
        
        ass_set_extract_fonts( pv->ssa, 1 );
        ass_set_style_overrides( pv->ssa, NULL );
        
        pv->renderer = ass_renderer_init( pv->ssa );
        if ( !pv->renderer ) {
            hb_log( "decssasub: renderer initialization failed\n" );
            return 1;
        }
        
        ass_set_use_margins( pv->renderer, 0 );
        ass_set_hinting( pv->renderer, ASS_HINTING_LIGHT );     // VLC 1.0.4 uses this
        ass_set_font_scale( pv->renderer, 1.0 );
        ass_set_line_spacing( pv->renderer, 1.0 );
        
        // Setup default font family
        // 
        // SSA v4.00 requires that "Arial" be the default font
        const char *font = NULL;
        const char *family = "Arial";
        // NOTE: This can sometimes block for several *seconds*.
        //       It seems that process_fontdata() for some embedded fonts is slow.
        ass_set_fonts( pv->renderer, font, family, /*haveFontConfig=*/1, NULL, 1 );
        
        // Setup track state
        pv->ssaTrack = ass_new_track( pv->ssa );
        if ( !pv->ssaTrack ) {
            hb_log( "decssasub: ssa track initialization failed\n" );
            return 1;
        }
        
        // NOTE: The codec extradata is expected to be in MKV format
        ass_process_codec_private( pv->ssaTrack,
            (char *) w->subtitle->extradata, w->subtitle->extradata_size );
        
        int originalWidth = job->title->width;
        int originalHeight = job->title->height;
        ass_set_frame_size( pv->renderer, originalWidth, originalHeight);
        ass_set_aspect_ratio( pv->renderer, /*dar=*/1.0, /*sar=*/1.0 );
    }
    
    return 0;
}

static int decssaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out_list = NULL;
    
    if ( in->size > 0 ) {
        out_list = ssa_decode_packet(w, in);
    } else {
        out_list = hb_buffer_init( 0 );
    }
    
    // Dispose the input packet, as it is no longer needed
    hb_buffer_close(&in);
    
    *buf_in = NULL;
    *buf_out = out_list;
    return HB_WORK_OK;
}

static void decssaClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv->ssaTrack )
        ass_free_track( pv->ssaTrack );
    if ( pv->renderer )
        ass_renderer_done( pv->renderer );
    if ( pv->ssa )
        ass_library_done( pv->ssa );
    
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
