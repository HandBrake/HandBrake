/* decssasub.c

   Copyright (c) 2003-2019 HandBrake Team
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
#include "handbrake/handbrake.h"

#include <ass/ass.h>
#include "handbrake/decssasub.h"
#include "handbrake/colormap.h"

struct hb_work_private_s
{
    hb_job_t      * job;
    hb_subtitle_t * subtitle;

    // SSA Import
    FILE          * file;
    int             readOrder;

    // Time of first desired subtitle adjusted by reader_pts_offset
    uint64_t start_time;
    uint64_t stop_time;
};

#define SSA_VERBOSE_PACKETS 0

static int extradataInit( hb_work_private_t * pv )
{
    int    events = 0;
    char * events_tag = "[Events]";
    char * format_tag = "Format:";
    int    events_len = strlen(events_tag);;
    int    format_len = strlen(format_tag);;
    char * header = NULL;

    while (1)
    {
        char    * line = NULL;
        ssize_t   len;
        size_t    size = 0;

        len = hb_getline(&line, &size, pv->file);
        if (len < 0)
        {
            // Incomplete SSA header
            free(header);
            return 1;
        }
        if (len > 0 && line != NULL)
        {
            if (header != NULL)
            {
                char * tmp = header;
                header = hb_strdup_printf("%s%s", header, line);
                free(tmp);
            }
            else
            {
                header = strdup(line);
            }
            if (!events)
            {
                if (len >= events_len &&
                    !strncasecmp(line, events_tag, events_len))
                {
                    events = 1;
                }
            }
            else
            {
                if (len >= format_len &&
                    !strncasecmp(line, format_tag, format_len))
                {
                    free(line);
                    break;
                }
                // Improperly formatted SSA header
                free(header);
                return 1;
            }
        }
        free(line);
    }
    pv->subtitle->extradata = (uint8_t*)header;
    pv->subtitle->extradata_size = strlen(header) + 1;

    return 0;
}

static int decssaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;
    int                 ii;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job         = job;
    pv->subtitle    = w->subtitle;

    if (w->fifo_in == NULL && pv->subtitle->config.src_filename != NULL)
    {
        pv->file = hb_fopen(pv->subtitle->config.src_filename, "r");
        if(pv->file == NULL)
        {
            hb_error("Could not open the SSA subtitle file '%s'\n",
                     pv->subtitle->config.src_filename);
            goto fail;
        }

        // Read SSA header and store in subtitle extradata
        if (extradataInit(pv))
        {
            goto fail;
        }
    }

    /*
     * Figure out the start and stop times from the chapters being
     * encoded - drop subtitle not in this range.
     */
    pv->start_time = 0;
    for (ii = 1; ii < job->chapter_start; ++ii)
    {
        hb_chapter_t * chapter = hb_list_item(job->list_chapter, ii - 1);
        if (chapter)
        {
            pv->start_time += chapter->duration;
        } else {
            hb_error("Could not locate chapter %d for SSA start time", ii);
        }
    }
    pv->stop_time = pv->start_time;
    for (ii = job->chapter_start; ii <= job->chapter_end; ++ii)
    {
        hb_chapter_t * chapter = hb_list_item(job->list_chapter, ii - 1);
        if (chapter)
        {
            pv->stop_time += chapter->duration;
        } else {
            hb_error("Could not locate chapter %d for SSA start time", ii);
        }
    }

    hb_deep_log(3, "SSA Start time %"PRId64", stop time %"PRId64,
                pv->start_time, pv->stop_time);

    if (job->pts_to_start != 0)
    {
        // Compute start_time after reader sets reader_pts_offset
        pv->start_time = AV_NOPTS_VALUE;
    }

    return 0;

fail:
    if (pv != NULL)
    {
        if (pv->file != NULL)
        {
            fclose(pv->file);
        }
        free(pv);
        w->private_data = NULL;
    }
    return 1;
}

#define SSA_2_HB_TIME(hr,min,sec,centi) \
    ( 90LL * ( hr    * 1000LL * 60 * 60 +\
              min   * 1000LL * 60 +\
              sec   * 1000LL +\
              centi * 10LL ) )

/*
 * Parses the start and stop time from the specified SSA packet.
 *
 * Returns true if parsing failed; false otherwise.
 */
static int parse_timing( char *line, int64_t *start, int64_t *stop )
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
    int numPartsRead = sscanf(line, "Dialogue:%*128[^,],"
        "%d:%d:%d.%d,"  // Start
        "%d:%d:%d.%d,", // End
        &start_hr, &start_min, &start_sec, &start_centi,
          &end_hr,   &end_min,   &end_sec,   &end_centi );
    if ( numPartsRead != 8 )
        return 1;

    *start = SSA_2_HB_TIME(start_hr, start_min, start_sec, start_centi);
    *stop  = SSA_2_HB_TIME(  end_hr,   end_min,   end_sec,   end_centi);

    return 0;
}

static char * find_field( char * pos, char * end, int fieldNum )
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
static hb_buffer_t *
decode_line_to_mkv_ssa( hb_work_private_t * pv, char * line, int size )
{
    hb_buffer_t * out;

    // Trim trailing CR/LF
    while (size > 0 && (line[size - 1] == '\n' || line[size - 1] == '\r'))
    {
        line[--size] = 0;
    }

    int64_t start, stop;
    if (parse_timing(line, &start, &stop))
    {
        goto fail;
    }

    // Convert the SSA packet to MKV-SSA format, which is what libass expects
    char * mkvSSA;
    int    numPartsRead;
    char * styleToTextFields;
    char * layerField = malloc(size);

    // SSA subtitles have an empty layer field (bare ',').  The scanf
    // format specifier "%*128[^,]" will not match on a bare ','.  There
    // must be at least one non ',' character in the match.  So the format
    // specifier is placed directly next to the ':' so that the next
    // expected ' ' after the ':' will be the character it matches on
    // when there is no layer field.
    numPartsRead = sscanf( (char *)line, "Dialogue:%128[^,],", layerField );
    if ( numPartsRead != 1 )
    {
        free(layerField);
        goto fail;
    }

    styleToTextFields = find_field( line, line + size, 4 );
    if ( styleToTextFields == NULL ) {
        free( layerField );
        goto fail;
    }

    // The sscanf conversion above will result in an extra space
    // before the layerField.  Strip the space.
    char *stripLayerField = layerField;
    for(; *stripLayerField == ' '; stripLayerField++);

    out = hb_buffer_init( size + 1 );
    mkvSSA = (char*)out->data;

    mkvSSA[0] = '\0';
    sprintf(mkvSSA, "%d", pv->readOrder++);
    strcat( mkvSSA, "," );
    strcat( mkvSSA, stripLayerField );
    strcat( mkvSSA, "," );
    strcat( mkvSSA, (char *)styleToTextFields );

    out->size           = strlen(mkvSSA) + 1;
    out->s.frametype    = HB_FRAME_SUBTITLE;
    out->s.start        = start + pv->subtitle->config.offset * 90;
    out->s.duration     = stop - start;
    out->s.stop         = stop + pv->subtitle->config.offset * 90;

    if (out->size == 0)
    {
        hb_buffer_close(&out);
    }
    else if (out->s.stop  <= pv->start_time ||
             out->s.start >= pv->stop_time)
    {
        // Drop subtitles that end before the start time
        // or start after the stop time
        hb_deep_log(3, "Discarding SSA at time start %"PRId64", stop %"PRId64,
                    out->s.start, out->s.stop);
        hb_buffer_close(&out);
    }
    else
    {
        if (out->s.start < pv->start_time)
        {
            out->s.start = pv->start_time;
        }
        if (out->s.stop > pv->stop_time)
        {
            out->s.stop = pv->stop_time;
        }
        out->s.start -= pv->start_time;
        out->s.stop  -= pv->start_time;
    }


    free( layerField );

    return out;

fail:
    hb_log( "decssasub: malformed SSA subtitle packet: %.*s\n", size, line );
    return NULL;
}

/*
 * Read the SSA file and put the entries into the subtitle fifo for all to read
 */
static hb_buffer_t * ssa_read( hb_work_private_t * pv )
{
    hb_buffer_t * out;

    if (pv->job->reader_pts_offset == AV_NOPTS_VALUE)
    {
        // We need to wait for reader to initialize it's pts offset so that
        // we know where to start reading SSA.
        return NULL;
    }
    if (pv->start_time == AV_NOPTS_VALUE)
    {
        pv->start_time = pv->job->reader_pts_offset;
        if (pv->job->pts_to_stop > 0)
        {
            pv->stop_time = pv->job->pts_to_start + pv->job->pts_to_stop;
        }
    }
    while (!feof(pv->file))
    {
        char    * line = NULL;
        ssize_t   len;
        size_t    size = 0;

        len = hb_getline(&line, &size, pv->file);
        if (len > 0 && line != NULL)
        {
            out = decode_line_to_mkv_ssa(pv, line, len);
            if (out != NULL)
            {
                free(line);
                return out;
            }
        }
        free(line);
        if (len < 0)
        {
            // Error or EOF
            out = hb_buffer_eof_init();
            return out;
        }
    }
    out = hb_buffer_eof_init();

    return out;
}

static int decssaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv =  w->private_data;
    hb_buffer_t       * in = *buf_in;

    *buf_in  = NULL;
    *buf_out = NULL;
    if (in == NULL && pv->file != NULL)
    {
        in = ssa_read(pv);
        if (in == NULL)
        {
            return HB_WORK_OK;
        }
    }
    *buf_out = in;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        return HB_WORK_DONE;
    }

    // Not much to do here.  ffmpeg already supplies SSA subtitles in the
    // required matroska packet format.
    //
    // We require string termination of the buffer
    hb_buffer_realloc(in, ++in->size);
    in->data[in->size - 1] = '\0';

#if SSA_VERBOSE_PACKETS
    printf("\nPACKET(%"PRId64",%"PRId64"): %.*s\n", in->s.start/90, in->s.stop/90, in->size, in->data);
#endif

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
