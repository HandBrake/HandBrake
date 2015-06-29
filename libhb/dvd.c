/* dvd.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "lang.h"
#include "dvd.h"

#include "dvdread/ifo_read.h"
#include "dvdread/ifo_print.h"
#include "dvdread/nav_read.h"

static hb_dvd_t    * hb_dvdread_init( hb_handle_t * h, char * path );
static void          hb_dvdread_close( hb_dvd_t ** _d );
static char        * hb_dvdread_name( char * path );
static int           hb_dvdread_title_count( hb_dvd_t * d );
static hb_title_t  * hb_dvdread_title_scan( hb_dvd_t * d, int t, uint64_t min_duration );
static int           hb_dvdread_start( hb_dvd_t * d, hb_title_t *title, int chapter );
static void          hb_dvdread_stop( hb_dvd_t * d );
static int           hb_dvdread_seek( hb_dvd_t * d, float f );
static hb_buffer_t * hb_dvdread_read( hb_dvd_t * d );
static int           hb_dvdread_chapter( hb_dvd_t * d );
static int           hb_dvdread_angle_count( hb_dvd_t * d );
static void          hb_dvdread_set_angle( hb_dvd_t * d, int angle );
static int           hb_dvdread_main_feature( hb_dvd_t * d, hb_list_t * list_title );

hb_dvd_func_t hb_dvdread_func =
{
    hb_dvdread_init,
    hb_dvdread_close,
    hb_dvdread_name,
    hb_dvdread_title_count,
    hb_dvdread_title_scan,
    hb_dvdread_start,
    hb_dvdread_stop,
    hb_dvdread_seek,
    hb_dvdread_read,
    hb_dvdread_chapter,
    hb_dvdread_angle_count,
    hb_dvdread_set_angle,
    hb_dvdread_main_feature
};

static hb_dvd_func_t *dvd_methods = &hb_dvdread_func;

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void FindNextCell( hb_dvdread_t * );
static int  dvdtime2msec( dvd_time_t * );
static int hb_dvdread_is_break( hb_dvdread_t * d );

hb_dvd_func_t * hb_dvdread_methods( void )
{
    return &hb_dvdread_func;
}

static int hb_dvdread_main_feature( hb_dvd_t * e, hb_list_t * list_title )
{
    int ii;
    uint64_t longest_duration = 0;
    int longest = -1;

    for ( ii = 0; ii < hb_list_count( list_title ); ii++ )
    {
        hb_title_t * title = hb_list_item( list_title, ii );
        if ( title->duration > longest_duration )
        {
            longest_duration = title->duration;
            longest = title->index;
        }
    }
    return longest;
}

static char * hb_dvdread_name( char * path )
{
    static char name[1024];
    unsigned char unused[1024];
    dvd_reader_t * reader;

    reader = DVDOpen( path );
    if( !reader )
    {
        return NULL;
    }

    if( DVDUDFVolumeInfo( reader, name, sizeof( name ),
                          unused, sizeof( unused ) ) )
    {
        DVDClose( reader );
        return NULL;
    }

    DVDClose( reader );
    return name;
}

/***********************************************************************
 * hb_dvdread_init
 ***********************************************************************
 *
 **********************************************************************/
hb_dvd_t * hb_dvdread_init( hb_handle_t * h, char * path )
{
    hb_dvd_t * e;
    hb_dvdread_t * d;
    int region_mask;
    char * path_ccp;

    e = calloc( sizeof( hb_dvd_t ), 1 );
    d = &(e->dvdread);
    d->h = h;

    /*
     * Convert UTF-8 path to current code page on Windows
     * hb_utf8_to_cp() is the same as strdup on non-Windows,
     * so no #ifdef required here
     */
    path_ccp = hb_utf8_to_cp( path );

	/* Log DVD drive region code */
    if ( hb_dvd_region( path_ccp, &region_mask ) == 0 )
    {
        hb_log( "dvd: Region mask 0x%02x", region_mask );
        if ( region_mask == 0xFF )
        {
            hb_log( "dvd: Warning, DVD device has no region set" );
        }
    }

    /* Open device */
    if( !( d->reader = DVDOpen( path_ccp ) ) )
    {
        /*
         * Not an error, may be a stream - which we'll try in a moment.
         */
        hb_log( "dvd: not a dvd - trying as a stream/file instead" );
        goto fail;
    }

    /* Open main IFO */
    if( !( d->vmg = ifoOpen( d->reader, 0 ) ) )
    {
        hb_log( "dvd: not a dvd - trying as a stream/file instead" );
        goto fail;
    }

    d->path = strdup( path ); /* hb_dvdread_title_scan assumes UTF-8 path, so not path_ccp here */
    free( path_ccp );

    return e;

fail:
    if( d->vmg )    ifoClose( d->vmg );
    if( d->reader ) DVDClose( d->reader );
    free( e );
    free( path_ccp );
    return NULL;
}

/***********************************************************************
 * hb_dvdread_title_count
 **********************************************************************/
static int hb_dvdread_title_count( hb_dvd_t * e )
{
    hb_dvdread_t *d = &(e->dvdread);
    return d->vmg->tt_srpt->nr_of_srpts;
}

/***********************************************************************
 * hb_dvdread_title_scan
 **********************************************************************/
static hb_title_t * hb_dvdread_title_scan( hb_dvd_t * e, int t, uint64_t min_duration )
{

    hb_dvdread_t *d = &(e->dvdread);
    hb_title_t   * title;
    ifo_handle_t * vts = NULL;
    int            pgc_id, pgn, i;
    hb_chapter_t * chapter;
    unsigned char  unused[1024];
    const char   * codec_name;

    hb_log( "scan: scanning title %d", t );

    title = hb_title_init( d->path, t );
    title->type = HB_DVD_TYPE;

    if( DVDUDFVolumeInfo( d->reader, title->name, sizeof( title->name ),
                          unused, sizeof( unused ) ) )
    {
        char * p_cur, * p_last = d->path;
        for( p_cur = d->path; *p_cur; p_cur++ )
        {
            if( IS_DIR_SEP(p_cur[0]) && p_cur[1] )
            {
                p_last = &p_cur[1];
            }
        }
        snprintf( title->name, sizeof( title->name ), "%s", p_last );
        char *dot_term = strrchr(title->name, '.');
        if (dot_term)
            *dot_term = '\0';
    }

    /* VTS which our title is in */
    title->vts = d->vmg->tt_srpt->title[t-1].title_set_nr;

    if ( !title->vts )
    {
        /* A VTS of 0 means the title wasn't found in the title set */
        hb_log("Invalid VTS (title set) number: %i", title->vts);
        goto fail;
    }

    hb_log( "scan: opening IFO for VTS %d", title->vts );
    if( !( vts = ifoOpen( d->reader, title->vts ) ) )
    {
        hb_log( "scan: ifoOpen failed" );
        goto fail;
    }

    /* ignore titles with bogus cell addresses so we don't abort later
     * in libdvdread. */
    for ( i = 0; i < vts->vts_c_adt->nr_of_vobs; ++i)
    {
        if( (vts->vts_c_adt->cell_adr_table[i].start_sector & 0xffffff ) ==
            0xffffff )
        {
            hb_log( "scan: cell_adr_table[%d].start_sector invalid (0x%x) "
                    "- skipping title", i,
                    vts->vts_c_adt->cell_adr_table[i].start_sector );
            goto fail;
        }
        if( (vts->vts_c_adt->cell_adr_table[i].last_sector & 0xffffff ) ==
            0xffffff )
        {
            hb_log( "scan: cell_adr_table[%d].last_sector invalid (0x%x) "
                    "- skipping title", i,
                    vts->vts_c_adt->cell_adr_table[i].last_sector );
            goto fail;
        }
        if( vts->vts_c_adt->cell_adr_table[i].start_sector >=
            vts->vts_c_adt->cell_adr_table[i].last_sector )
        {
            hb_log( "scan: cell_adr_table[%d].start_sector (0x%x) "
                    "is not before last_sector (0x%x) - skipping title", i,
                    vts->vts_c_adt->cell_adr_table[i].start_sector,
                    vts->vts_c_adt->cell_adr_table[i].last_sector );
            goto fail;
        }
    }

    if( global_verbosity_level == 3 )
    {
        ifo_print( d->reader, title->vts );
    }

    /* Position of the title in the VTS */
    title->ttn = d->vmg->tt_srpt->title[t-1].vts_ttn;
    if ( title->ttn < 1 || title->ttn > vts->vts_ptt_srpt->nr_of_srpts )
    {
        hb_log( "invalid VTS PTT offset %d for title %d, skipping", title->ttn, t );
        goto fail;
    }

    /* Get pgc */
    pgc_id = vts->vts_ptt_srpt->title[title->ttn-1].ptt[0].pgcn;
    if ( pgc_id < 1 || pgc_id > vts->vts_pgcit->nr_of_pgci_srp )
    {
        hb_log( "invalid PGC ID %d for title %d, skipping", pgc_id, t );
        goto fail;
    }
    pgn    = vts->vts_ptt_srpt->title[title->ttn-1].ptt[0].pgn;
    d->pgc = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;

    hb_log("pgc_id: %d, pgn: %d: pgc: %p", pgc_id, pgn, d->pgc);

    if( !d->pgc || !d->pgc->program_map )
    {
        hb_log( "scan: pgc not valid, skipping" );
        goto fail;
    }

    if (d->pgc->cell_playback == NULL)
    {
        hb_log( "invalid PGC cell_playback table for title %d, skipping", t );
        goto fail;
    }

    if( pgn <= 0 || pgn > 99 )
    {
        hb_log( "scan: pgn %d not valid, skipping", pgn );
        goto fail;
    }

    /* Start cell */
    title->cell_start  = d->pgc->program_map[pgn-1] - 1;
    title->block_start = d->pgc->cell_playback[title->cell_start].first_sector;

    /* End cell */
    title->cell_end  = d->pgc->nr_of_cells - 1;
    title->block_end = d->pgc->cell_playback[title->cell_end].last_sector;

    /* Block count */
    title->block_count = 0;
    d->cell_cur = title->cell_start;
    while( d->cell_cur <= title->cell_end )
    {
#define cp d->pgc->cell_playback[d->cell_cur]
        title->block_count += cp.last_sector + 1 - cp.first_sector;
#undef cp
        FindNextCell( d );
        d->cell_cur = d->cell_next;
    }

    hb_log( "scan: vts=%d, ttn=%d, cells=%d->%d, blocks=%"PRIu64"->%"PRIu64", "
            "%"PRIu64" blocks", title->vts, title->ttn, title->cell_start,
            title->cell_end, title->block_start, title->block_end,
            title->block_count );

    /* Get duration */
    title->duration = 90LL * dvdtime2msec( &d->pgc->playback_time );
    title->hours    = title->duration / 90000 / 3600;
    title->minutes  = ( ( title->duration / 90000 ) % 3600 ) / 60;
    title->seconds  = ( title->duration / 90000 ) % 60;
    hb_log( "scan: duration is %02d:%02d:%02d (%"PRId64" ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* ignore titles under 10 seconds because they're often stills or
     * clips with no audio & our preview code doesn't currently handle
     * either of these. */
    if( title->duration < min_duration )
    {
        hb_log( "scan: ignoring title (too short)" );
        goto fail;
    }

    /* Detect languages */
    for( i = 0; i < vts->vtsi_mat->nr_of_vts_audio_streams; i++ )
    {
        int audio_format, lang_code, lang_extension, audio_control, position, j;
        hb_audio_t * audio, * audio_tmp;
        iso639_lang_t * lang;

        hb_log( "scan: checking audio %d", i + 1 );

        audio = calloc( sizeof( hb_audio_t ), 1 );

        audio_format  = vts->vtsi_mat->vts_audio_attr[i].audio_format;
        lang_code     = vts->vtsi_mat->vts_audio_attr[i].lang_code;
        lang_extension = vts->vtsi_mat->vts_audio_attr[i].code_extension;
        audio_control =
            vts->vts_pgcit->pgci_srp[pgc_id-1].pgc->audio_control[i];

        if( !( audio_control & 0x8000 ) )
        {
            hb_log( "scan: audio channel is not active" );
            free( audio );
            continue;
        }

        position = ( audio_control & 0x7F00 ) >> 8;

        switch( audio_format )
        {
            case 0x00:
                audio->id    = ( ( 0x80 + position ) << 8 ) | 0xbd;
                audio->config.in.codec = HB_ACODEC_AC3;
                audio->config.in.codec_param = AV_CODEC_ID_AC3;
                codec_name = "AC3";
                break;

            case 0x02:
            case 0x03:
                audio->id    = 0xc0 + position;
                audio->config.in.codec = HB_ACODEC_FFMPEG;
                audio->config.in.codec_param = AV_CODEC_ID_MP2;
                codec_name = "MPEG";
                break;

            case 0x04:
                audio->id    = ( ( 0xa0 + position ) << 8 ) | 0xbd;
                audio->config.in.codec = HB_ACODEC_LPCM;
                codec_name = "LPCM";
                break;

            case 0x06:
                audio->id    = ( ( 0x88 + position ) << 8 ) | 0xbd;
                audio->config.in.codec = HB_ACODEC_DCA;
                audio->config.in.codec_param = AV_CODEC_ID_DTS;
                codec_name = "DTS";
                break;

            default:
                audio->id    = 0;
                audio->config.in.codec = 0;
                codec_name = "Unknown";
                hb_log( "scan: unknown audio codec (%x)",
                        audio_format );
                break;
        }
        if( !audio->id )
        {
            continue;
        }

        /* Check for duplicate tracks */
        audio_tmp = NULL;
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            audio_tmp = hb_list_item( title->list_audio, j );
            if( audio->id == audio_tmp->id )
            {
                break;
            }
            audio_tmp = NULL;
        }
        if( audio_tmp )
        {
            hb_log( "scan: duplicate audio track" );
            free( audio );
            continue;
        }

        lang = lang_for_code( lang_code );

        audio->config.lang.type = lang_extension;

        snprintf( audio->config.lang.simple,
                  sizeof( audio->config.lang.simple ), "%s",
                  strlen( lang->native_name ) ? lang->native_name : lang->eng_name );
        snprintf( audio->config.lang.iso639_2,
                  sizeof( audio->config.lang.iso639_2 ), "%s", lang->iso639_2 );

        hb_log("scan: id=0x%x, lang=%s (%s), 3cc=%s ext=%i", audio->id,
               audio->config.lang.simple, codec_name,
               audio->config.lang.iso639_2, lang_extension);

        audio->config.in.track = i;
        hb_list_add( title->list_audio, audio );
    }

    /* Check for subtitles */
    for( i = 0; i < vts->vtsi_mat->nr_of_vts_subp_streams; i++ )
    {
        hb_subtitle_t * subtitle;
        int spu_control;
        int position;
        iso639_lang_t * lang;
        int lang_extension = 0;

        hb_log( "scan: checking subtitle %d", i + 1 );

        spu_control =
            vts->vts_pgcit->pgci_srp[pgc_id-1].pgc->subp_control[i];

        if( !( spu_control & 0x80000000 ) )
        {
            hb_log( "scan: subtitle channel is not active" );
            continue;
        }

        if( vts->vtsi_mat->vts_video_attr.display_aspect_ratio )
        {
            switch( vts->vtsi_mat->vts_video_attr.permitted_df )
            {
                case 1:
                    position = spu_control & 0xFF;
                    break;
                case 2:
                    position = ( spu_control >> 8 ) & 0xFF;
                    break;
                default:
                    position = ( spu_control >> 16 ) & 0xFF;
            }
        }
        else
        {
            position = ( spu_control >> 24 ) & 0x7F;
        }

        lang_extension = vts->vtsi_mat->vts_subp_attr[i].code_extension;

        lang = lang_for_code( vts->vtsi_mat->vts_subp_attr[i].lang_code );

        subtitle = calloc( sizeof( hb_subtitle_t ), 1 );
        subtitle->track = i;
        subtitle->id = ( ( 0x20 + position ) << 8 ) | 0xbd;
        snprintf( subtitle->lang, sizeof( subtitle->lang ), "%s",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name);
        snprintf( subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "%s",
                  lang->iso639_2);
        subtitle->format = PICTURESUB;
        subtitle->source = VOBSUB;
        subtitle->config.dest   = RENDERSUB;  // By default render (burn-in) the VOBSUB.
        subtitle->stream_type = 0xbd;
        subtitle->substream_type = 0x20 + position;
        subtitle->codec = WORK_DECVOBSUB;

        subtitle->type = lang_extension;
        
        memcpy( subtitle->palette,
            vts->vts_pgcit->pgci_srp[pgc_id-1].pgc->palette,
            16 * sizeof( uint32_t ) );
        subtitle->palette_set = 1;

        switch( lang_extension )
        {  
            case 2:
                strcat( subtitle->lang, " (Caption with bigger size character)" );
                break;
            case 3: 
                strcat( subtitle->lang, " (Caption for Children)" );
                break;
            case 5:
                strcat( subtitle->lang, " (Closed Caption)" );
                break;
            case 6:
                strcat( subtitle->lang, " (Closed Caption with bigger size character)" );
                break;
            case 7:
                strcat( subtitle->lang, " (Closed Caption for Children)" );
                break;
            case 9:
                strcat( subtitle->lang, " (Forced Caption)" );
                break;
            case 13:
                strcat( subtitle->lang, " (Director's Commentary)" );
                break;
            case 14:
                strcat( subtitle->lang, " (Director's Commentary with bigger size character)" );
                break;
            case 15:
                strcat( subtitle->lang, " (Director's Commentary for Children)" );
            default:
                break;
        }

        hb_log( "scan: id=0x%x, lang=%s, 3cc=%s ext=%i", subtitle->id,
                subtitle->lang, subtitle->iso639_2, lang_extension );

        hb_list_add( title->list_subtitle, subtitle );
    }

    /* Chapters */
    hb_log( "scan: title %d has %d chapters", t,
            vts->vts_ptt_srpt->title[title->ttn-1].nr_of_ptts );
    for( i = 0;
         i < vts->vts_ptt_srpt->title[title->ttn-1].nr_of_ptts; i++ )
    {
        char chapter_title[80];
        chapter = calloc( sizeof( hb_chapter_t ), 1 );

        /* remember the on-disc chapter number */
        chapter->index = i + 1;
        sprintf( chapter_title, "Chapter %d", chapter->index );
        hb_chapter_set_title( chapter, chapter_title );

        pgc_id = vts->vts_ptt_srpt->title[title->ttn-1].ptt[i].pgcn;
        pgn    = vts->vts_ptt_srpt->title[title->ttn-1].ptt[i].pgn;
        d->pgc = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;

        /* Start cell */
        chapter->cell_start  = d->pgc->program_map[pgn-1] - 1;
        chapter->block_start =
            d->pgc->cell_playback[chapter->cell_start].first_sector;

        // if there are no more programs in this pgc, the end cell is the
        // last cell. Otherwise it's the cell before the start cell of the
        // next program.
        if ( pgn == d->pgc->nr_of_programs )
        {
            chapter->cell_end = d->pgc->nr_of_cells - 1;
        }
        else
        {
            chapter->cell_end = d->pgc->program_map[pgn] - 2;;
        }
        chapter->block_end = d->pgc->cell_playback[chapter->cell_end].last_sector;

        /* Block count, duration */
        chapter->block_count = 0;
        chapter->duration = 0;

        d->cell_cur = chapter->cell_start;
        while( d->cell_cur <= chapter->cell_end )
        {
#define cp d->pgc->cell_playback[d->cell_cur]
            chapter->block_count += cp.last_sector + 1 - cp.first_sector;
            chapter->duration += 90LL * dvdtime2msec( &cp.playback_time );
#undef cp
            FindNextCell( d );
            d->cell_cur = d->cell_next;
        }

        hb_list_add( title->list_chapter, chapter );
    }

    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        chapter           = hb_list_item( title->list_chapter, i );

        int seconds       = ( chapter->duration + 45000 ) / 90000;
        chapter->hours    = ( seconds / 3600 );
        chapter->minutes  = ( seconds % 3600 ) / 60;
        chapter->seconds  = ( seconds % 60 );

        hb_log( "scan: chap %d c=%d->%d, b=%"PRIu64"->%"PRIu64" (%"PRIu64"), %"PRId64" ms",
                chapter->index, chapter->cell_start, chapter->cell_end,
                chapter->block_start, chapter->block_end,
                chapter->block_count, chapter->duration / 90 );
    }

    /* Get aspect. We don't get width/height/rate infos here as
       they tend to be wrong */
    switch( vts->vtsi_mat->vts_video_attr.display_aspect_ratio )
    {
        case 0:
            title->container_dar.num = 4;
            title->container_dar.den = 3;
            break;
        case 3:
            title->container_dar.num = 16;
            title->container_dar.den = 9;
            break;
        default:
            hb_log( "scan: unknown aspect" );
            goto fail;
    }

    hb_log("scan: aspect = %d:%d",
           title->container_dar.num, title->container_dar.den);

    /* This title is ok so far */
    goto cleanup;

fail:
    hb_title_close( &title );

cleanup:
    if( vts ) ifoClose( vts );

    return title;
}

/***********************************************************************
 * hb_dvdread_start
 ***********************************************************************
 * Title and chapter start at 1
 **********************************************************************/
static int hb_dvdread_start( hb_dvd_t * e, hb_title_t *title, int chapter )
{
    hb_dvdread_t *d = &(e->dvdread);
    int pgc_id, pgn;
    int i;
    int t = title->index;

    /* Open the IFO and the VOBs for this title */
    d->vts = d->vmg->tt_srpt->title[t-1].title_set_nr;
    d->ttn = d->vmg->tt_srpt->title[t-1].vts_ttn;
    if( !( d->ifo = ifoOpen( d->reader, d->vts ) ) )
    {
        hb_error( "dvd: ifoOpen failed for VTS %d", d->vts );
        return 0;
    }
    if( !( d->file = DVDOpenFile( d->reader, d->vts,
                                  DVD_READ_TITLE_VOBS ) ) )
    {
        hb_error( "dvd: DVDOpenFile failed for VTS %d", d->vts );
        return 0;
    }

    /* Get title first and last blocks */
    pgc_id         = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[0].pgcn;
    pgn            = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[0].pgn;
    d->pgc         = d->ifo->vts_pgcit->pgci_srp[pgc_id-1].pgc;
    d->cell_start  = d->pgc->program_map[pgn - 1] - 1;
    d->cell_end    = d->pgc->nr_of_cells - 1;
    d->title_start = d->pgc->cell_playback[d->cell_start].first_sector;
    d->title_end   = d->pgc->cell_playback[d->cell_end].last_sector;

    /* Block count */
    d->title_block_count = 0;
    for( i = d->cell_start; i <= d->cell_end; i++ )
    {
        d->title_block_count += d->pgc->cell_playback[i].last_sector + 1 -
            d->pgc->cell_playback[i].first_sector;
    }

    /* Get pgc for the current chapter */
    pgc_id = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[chapter-1].pgcn;
    pgn    = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[chapter-1].pgn;
    d->pgc = d->ifo->vts_pgcit->pgci_srp[pgc_id-1].pgc;

    /* Get the two first cells */
    d->cell_cur = d->pgc->program_map[pgn-1] - 1;
    FindNextCell( d );

    d->block     = d->pgc->cell_playback[d->cell_cur].first_sector;
    d->next_vobu = d->block;
    d->pack_len  = 0;
    d->cell_overlap = 0;
    d->in_cell = 0;
    d->in_sync = 2;

    return 1;
}

/***********************************************************************
 * hb_dvdread_stop
 ***********************************************************************
 *
 **********************************************************************/
static void hb_dvdread_stop( hb_dvd_t * e )
{
    hb_dvdread_t *d = &(e->dvdread);
    if( d->ifo )
    {
        ifoClose( d->ifo );
        d->ifo = NULL;
    }
    if( d->file )
    {
        DVDCloseFile( d->file );
        d->file = NULL;
    }
}

/***********************************************************************
 * hb_dvdread_seek
 ***********************************************************************
 *
 **********************************************************************/
static int hb_dvdread_seek( hb_dvd_t * e, float f )
{
    hb_dvdread_t *d = &(e->dvdread);
    int count, sizeCell;
    int i;

    count = f * d->title_block_count;

    for( i = d->cell_start; i <= d->cell_end; i++ )
    {
        sizeCell = d->pgc->cell_playback[i].last_sector + 1 -
            d->pgc->cell_playback[i].first_sector;

        if( count < sizeCell )
        {
            d->cell_cur = i;
            d->cur_cell_id = 0;
            FindNextCell( d );

            /* Now let hb_dvdread_read find the next VOBU */
            d->next_vobu = d->pgc->cell_playback[i].first_sector + count;
            d->pack_len  = 0;
            break;
        }

        count -= sizeCell;
    }

    if( i > d->cell_end )
    {
        return 0;
    }

    /*
     * Assume that we are in sync, even if we are not given that it is obvious
     * that we are out of sync if we have just done a seek.
     */
    d->in_sync = 2;

    return 1;
}


/***********************************************************************
 * is_nav_pack
 ***********************************************************************/
int is_nav_pack( unsigned char *buf )
{
    /*
     * The NAV Pack is comprised of the PCI Packet and DSI Packet, both
     * of these start at known offsets and start with a special identifier.
     *
     * NAV = {
     *  PCI = { 00 00 01 bf  # private stream header
     *          ?? ??        # length
     *          00           # substream
     *          ...
     *        }
     *  DSI = { 00 00 01 bf  # private stream header
     *          ?? ??        # length
     *          01           # substream
     *          ...
     *        }
     *
     * The PCI starts at offset 0x26 into the sector, and the DSI starts at 0x400
     *
     * This information from: http://dvd.sourceforge.net/dvdinfo/
     */
    if( ( buf[0x26] == 0x00 &&      // PCI
          buf[0x27] == 0x00 &&
          buf[0x28] == 0x01 &&
          buf[0x29] == 0xbf &&
          buf[0x2c] == 0x00 ) &&
        ( buf[0x400] == 0x00 &&     // DSI
          buf[0x401] == 0x00 &&
          buf[0x402] == 0x01 &&
          buf[0x403] == 0xbf &&
          buf[0x406] == 0x01 ) )
    {
        return ( 1 );
    } else {
        return ( 0 );
    }
}

/***********************************************************************
 * hb_dvdread_read
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * hb_dvdread_read( hb_dvd_t * e )
{
    hb_dvdread_t *d = &(e->dvdread);
    hb_buffer_t *b = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );
 top:
    if( !d->pack_len )
    {
        /* New pack */
        dsi_t dsi_pack;
        int   error = 0;

        // if we've just finished the last cell of the title we don't
        // want to read another block because our next_vobu pointer
        // is probably invalid. Just return 'no data' & our caller
        // should check and discover we're at eof.
        if ( d->cell_cur > d->cell_end )
        {
            hb_buffer_close( &b );
            return NULL;
        }

        for( ;; )
        {
            int block, pack_len, next_vobu, read_retry;

            for( read_retry = 1; read_retry < 1024; read_retry++ )
            {
                if( DVDReadBlocks( d->file, d->next_vobu, 1, b->data ) == 1 )
                {
                    /*
                     * Successful read.
                     */
                    if( read_retry > 1 && !is_nav_pack( b->data) )
                    {
                        // But wasn't a nav pack, so carry on looking
                        read_retry = 1;
                        d->next_vobu++;
                        continue;
                    }
                    break;
                } else {  
                    // First retry the same block, then try the next one, 
                    // adjust the skip increment upwards so that we can skip
                    // large sections of bad blocks more efficiently (at the
                    // cost of some missed good blocks at the end).
                    hb_log( "dvd: vobu read error blk %d - skipping to next blk incr %d",
                            d->next_vobu, (read_retry * 10));
                    d->next_vobu += (read_retry * 10);
                }
                
            }

            if( read_retry == 1024 )
            {
                // That's too many bad blocks, jump to the start of the
                // next cell.
                hb_log( "dvd: vobu read error blk %d - skipping to cell %d",
                        d->next_vobu, d->cell_next );
                d->cell_cur  = d->cell_next;
                if ( d->cell_cur > d->cell_end )
                {
                    hb_buffer_close( &b );
                    hb_set_work_error(d->h, HB_ERROR_READ);
                    return NULL;
                }
                d->in_cell = 0;
                d->next_vobu = d->pgc->cell_playback[d->cell_cur].first_sector;
                FindNextCell( d );
                d->cell_overlap = 1;
                continue;
            }

            if ( !is_nav_pack( b->data ) ) {
                (d->next_vobu)++;
                if( d->in_sync == 1 ) {
                    hb_log("dvd: Lost sync, searching for NAV pack at blk %d",
                           d->next_vobu);
                    d->in_sync = 0;
                } 
                continue;
            }

            navRead_DSI( &dsi_pack, &b->data[DSI_START_BYTE] );

            if ( d->in_sync == 0 && d->cur_cell_id &&
                 (d->cur_vob_id != dsi_pack.dsi_gi.vobu_vob_idn ||
                  d->cur_cell_id != dsi_pack.dsi_gi.vobu_c_idn ) )
            {
                // We walked out of the cell we're supposed to be in.
                // If we're not at the start of our next cell go there.
                hb_log("dvd: left cell %d (%u,%u) for (%u,%u) at block %u",
                       d->cell_cur, d->cur_vob_id, d->cur_cell_id,
                       dsi_pack.dsi_gi.vobu_vob_idn, dsi_pack.dsi_gi.vobu_c_idn,
                       d->next_vobu );
                if ( d->next_vobu != d->pgc->cell_playback[d->cell_next].first_sector )
                {
                    d->next_vobu = d->pgc->cell_playback[d->cell_next].first_sector;
                    d->cur_cell_id = 0;
                    continue;
                }
            }

            block     = dsi_pack.dsi_gi.nv_pck_lbn;
            pack_len  = dsi_pack.dsi_gi.vobu_ea;

            // There are a total of 21 next vobu offsets (and 21 prev_vobu
            // offsets) in the navpack SRI structure. The primary one is
            // 'next_vobu' which is the offset (in dvd blocks) from the current
            // block to the start of the next vobu. If the block at 'next_vobu'
            // can't be read, 'next_video' is the offset to the vobu closest to it.
            // The other 19 offsets are vobus at successively longer distances from
            // the current block (this is so that a hardware player can do
            // adaptive error recovery to skip over a bad spot on the disk). In all
            // these offsets the high bit is set to indicate when it contains a
            // valid offset. The next bit (2^30) is set to indicate that there's
            // another valid offset in the SRI that's closer to the current block.
            // A hardware player uses these bits to chose the closest valid offset
            // and uses that as its next vobu. (Some mastering schemes appear to
            // put a bogus offset in next_vobu with the 2^30 bit set & the
            // correct offset in next_video. This works fine in hardware players
            // but will mess up software that doesn't implement the full next
            // vobu decision logic.) In addition to the flag bits there's a
            // reserved value of the offset that indicates 'no next vobu' (which
            // indicates the end of a cell). But having no next vobu pointer with a
            // 'valid' bit set also indicates end of cell. Different mastering
            // schemes seem to use all possible combinations of the flag bits
            // and reserved values to indicate end of cell so we have to check
            // them all or we'll get a disk read error from following an
            // invalid offset.
            uint32_t next_ptr = dsi_pack.vobu_sri.next_vobu;
            if ( ( next_ptr & ( 1 << 31 ) ) == 0  ||
                 ( next_ptr & ( 1 << 30 ) ) != 0 ||
                 ( next_ptr & 0x3fffffff ) == 0x3fffffff )
            {
                next_ptr = dsi_pack.vobu_sri.next_video;
                if ( ( next_ptr & ( 1 << 31 ) ) == 0 ||
                     ( next_ptr & 0x3fffffff ) == 0x3fffffff )
                {
                    // don't have a valid forward pointer - assume end-of-cell
                    d->block     = block;
                    d->pack_len  = pack_len;
                    break;
                }
            }
            next_vobu = block + ( next_ptr & 0x3fffffff );

            if( pack_len >  0    &&
                pack_len <  1024 &&
                block    >= d->next_vobu &&
                ( block <= d->title_start + d->title_block_count ||
                  block <= d->title_end ) )
            {
                d->block     = block;
                d->pack_len  = pack_len;
                d->next_vobu = next_vobu;
                break;
            }

            /* Wasn't a valid VOBU, try next block */
            if( ++error > 1024 )
            {
                hb_error( "dvd: couldn't find a VOBU after 1024 blocks" );
                hb_buffer_close( &b );
                hb_set_work_error(d->h, HB_ERROR_READ);
                return NULL;
            }

            (d->next_vobu)++;
        }

        if( d->in_sync == 0 || d->in_sync == 2 )
        {
            if( d->in_sync == 0 )
            {
                hb_log( "dvd: In sync with DVD at block %d", d->block );
            }
            d->in_sync = 1;
        }

        // Revert the cell overlap, and check for a chapter break
        // If this cell is zero length (prev_vobu & next_vobu both
        // set to END_OF_CELL) we need to check for beginning of
        // cell before checking for end or we'll advance to the next
        // cell too early and fail to generate a chapter mark when this
        // cell starts a chapter.
        if( ( dsi_pack.vobu_sri.prev_vobu & (1 << 31 ) ) == 0 ||
            ( dsi_pack.vobu_sri.prev_vobu & 0x3fffffff ) == 0x3fffffff )
        {
            // A vobu that's not at the start of a cell can have an
            // EOC prev pointer (this seems to be associated with some
            // sort of drm). The rest of the content in the cell may be
            // booby-trapped so treat this like an end-of-cell rather
            // than a beginning of cell.
            if ( d->pgc->cell_playback[d->cell_cur].first_sector < dsi_pack.dsi_gi.nv_pck_lbn &&
                 d->pgc->cell_playback[d->cell_cur].last_sector >= dsi_pack.dsi_gi.nv_pck_lbn )
            {
                hb_log( "dvd: null prev_vobu in cell %d at block %d", d->cell_cur,
                        d->block );
                // treat like end-of-cell then go directly to start of next cell.
                d->cell_cur  = d->cell_next;
                d->in_cell = 0;
                d->next_vobu = d->pgc->cell_playback[d->cell_cur].first_sector;
                FindNextCell( d );
                d->cell_overlap = 1;
                goto top;
            }
            else
            {
                if ( d->block != d->pgc->cell_playback[d->cell_cur].first_sector )
                {
                    hb_log( "dvd: beginning of cell %d at block %d", d->cell_cur,
                           d->block );
                }
                if( d->in_cell )
                {
                    hb_error( "dvd: assuming missed end of cell %d at block %d", d->cell_cur, d->block );
                    d->cell_cur  = d->cell_next;
                    d->next_vobu = d->pgc->cell_playback[d->cell_cur].first_sector;
                    FindNextCell( d );
                    d->cell_overlap = 1;
                    d->in_cell = 0;
                } else {
                    d->in_cell = 1;
                }
                d->cur_vob_id = dsi_pack.dsi_gi.vobu_vob_idn;
                d->cur_cell_id = dsi_pack.dsi_gi.vobu_c_idn;

                if( d->cell_overlap )
                {
                    b->s.new_chap = hb_dvdread_is_break( d );
                    d->cell_overlap = 0;
                }
            }
        }

        if( ( dsi_pack.vobu_sri.next_vobu & (1 << 31 ) ) == 0 ||
            ( dsi_pack.vobu_sri.next_vobu & 0x3fffffff ) == 0x3fffffff )
        {
            if ( d->block <= d->pgc->cell_playback[d->cell_cur].first_sector ||
                 d->block > d->pgc->cell_playback[d->cell_cur].last_sector )
            {
                hb_log( "dvd: end of cell %d at block %d", d->cell_cur,
                        d->block );
            }
            d->cell_cur  = d->cell_next;
            d->in_cell = 0;
            d->next_vobu = d->pgc->cell_playback[d->cell_cur].first_sector;
            FindNextCell( d );
            d->cell_overlap = 1;

        }
    }
    else
    {
        if( DVDReadBlocks( d->file, d->block, 1, b->data ) != 1 )
        {
            // this may be a real DVD error or may be DRM. Either way
            // we don't want to quit because of one bad block so set
            // things up so we'll advance to the next vobu and recurse.
            hb_error( "dvd: DVDReadBlocks failed (%d), skipping to vobu %u",
                      d->block, d->next_vobu );
            d->pack_len = 0;
            goto top;  /* XXX need to restructure this routine & avoid goto */
        }
        d->pack_len--;
    }

    d->block++;

    return b;
}

/***********************************************************************
 * hb_dvdread_chapter
 ***********************************************************************
 * Returns in which chapter the next block to be read is.
 * Chapter numbers start at 1.
 **********************************************************************/
static int hb_dvdread_chapter( hb_dvd_t * e )
{
    hb_dvdread_t *d = &(e->dvdread);
    int     i;
    int     pgc_id, pgn;
    int     nr_of_ptts = d->ifo->vts_ptt_srpt->title[d->ttn-1].nr_of_ptts;
    pgc_t * pgc;

    for( i = nr_of_ptts - 1;
         i >= 0;
         i-- )
    {
        /* Get pgc for chapter (i+1) */
        pgc_id = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[i].pgcn;
        pgn    = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[i].pgn;
        pgc    = d->ifo->vts_pgcit->pgci_srp[pgc_id-1].pgc;

        if( d->cell_cur - d->cell_overlap >= pgc->program_map[pgn-1] - 1 &&
            d->cell_cur - d->cell_overlap <= pgc->nr_of_cells - 1 )
        {
            /* We are in this chapter */
            return i + 1;
        }
    }

    /* End of title */
    return -1;
}

/***********************************************************************
 * hb_dvdread_is_break
 ***********************************************************************
 * Returns chapter number if the current block is a new chapter start
 **********************************************************************/
static int hb_dvdread_is_break( hb_dvdread_t * d )
{
    int     i;
    int     pgc_id, pgn;
    int     nr_of_ptts = d->ifo->vts_ptt_srpt->title[d->ttn-1].nr_of_ptts;
    pgc_t * pgc;
    int     cell;

    for( i = nr_of_ptts - 1;
         i > 0;
         i-- )
    {
        /* Get pgc for chapter (i+1) */
        pgc_id = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[i].pgcn;
        pgn    = d->ifo->vts_ptt_srpt->title[d->ttn-1].ptt[i].pgn;
        pgc    = d->ifo->vts_pgcit->pgci_srp[pgc_id-1].pgc;
        cell   = pgc->program_map[pgn-1] - 1;

        if( cell <= d->cell_start )
            break;

        // This must not match against the start cell.
        if( pgc->cell_playback[cell].first_sector == d->block && cell != d->cell_start )
        {
            return i + 1;
        }
    }

    return 0;
}

/***********************************************************************
 * hb_dvdread_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
static void hb_dvdread_close( hb_dvd_t ** _d )
{
    hb_dvdread_t * d = &((*_d)->dvdread);

    if( d->vmg )
    {
        ifoClose( d->vmg );
    }
    if( d->reader )
    {
        DVDClose( d->reader );
    }

    free( d );
    *_d = NULL;
}

/***********************************************************************
 * hb_dvdread_angle_count
 ***********************************************************************
 * Returns the number of angles supported.  We do not support angles
 * with dvdread
 **********************************************************************/
static int hb_dvdread_angle_count( hb_dvd_t * d )
{
    return 1;
}

/***********************************************************************
 * hb_dvdread_set_angle
 ***********************************************************************
 * Sets the angle to read.  Not supported with dvdread
 **********************************************************************/
static void hb_dvdread_set_angle( hb_dvd_t * d, int angle )
{
}

/***********************************************************************
 * FindNextCell
 ***********************************************************************
 * Assumes pgc and cell_cur are correctly set, and sets cell_next to the
 * cell to be read when we will be done with cell_cur.
 **********************************************************************/
static void FindNextCell( hb_dvdread_t * d )
{
    int i = 0;

    if( d->pgc->cell_playback[d->cell_cur].block_type ==
            BLOCK_TYPE_ANGLE_BLOCK )
    {

        while( d->pgc->cell_playback[d->cell_cur+i].block_mode !=
                   BLOCK_MODE_LAST_CELL )
        {
             i++;
        }
        d->cell_next = d->cell_cur + i + 1;
        hb_log( "dvd: Skipping multi-angle cells %d-%d",
                d->cell_cur,
                d->cell_next - 1 );
    }
    else
    {
        d->cell_next = d->cell_cur + 1;
    }
}

/***********************************************************************
 * dvdtime2msec
 ***********************************************************************
 * From lsdvd
 **********************************************************************/
static int dvdtime2msec(dvd_time_t * dt)
{
    double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};
    double fps = frames_per_s[(dt->frame_u & 0xc0) >> 6];
    long   ms;
    ms  = (((dt->hour &   0xf0) >> 3) * 5 + (dt->hour   & 0x0f)) * 3600000;
    ms += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60000;
    ms += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f)) * 1000;

    if( fps > 0 )
    {
        ms += (((dt->frame_u & 0x30) >> 3) * 5 +
                (dt->frame_u & 0x0f)) * 1000.0 / fps;
    }

    return ms;
}

char * hb_dvd_name( char * path )
{
    return dvd_methods->name(path);
}

hb_dvd_t * hb_dvd_init( hb_handle_t * h, char * path )
{
    return dvd_methods->init(h, path);
}

int hb_dvd_title_count( hb_dvd_t * d )
{
    return dvd_methods->title_count(d);
}

hb_title_t * hb_dvd_title_scan( hb_dvd_t * d, int t, uint64_t min_duration )
{
    return dvd_methods->title_scan(d, t, min_duration);
}

int hb_dvd_start( hb_dvd_t * d, hb_title_t *title, int chapter )
{
    return dvd_methods->start(d, title, chapter);
}

void hb_dvd_stop( hb_dvd_t * d )
{
    dvd_methods->stop(d);
}

int hb_dvd_seek( hb_dvd_t * d, float f )
{
    return dvd_methods->seek(d, f);
}

hb_buffer_t * hb_dvd_read( hb_dvd_t * d )
{
    return dvd_methods->read(d);
}

int hb_dvd_chapter( hb_dvd_t * d )
{
    return dvd_methods->chapter(d);
}

void hb_dvd_close( hb_dvd_t ** _d )
{
    dvd_methods->close(_d);
}

int hb_dvd_angle_count( hb_dvd_t * d )
{
    return dvd_methods->angle_count(d);
}

void hb_dvd_set_angle( hb_dvd_t * d, int angle )
{
    dvd_methods->set_angle(d, angle);
}

int hb_dvd_main_feature( hb_dvd_t * d, hb_list_t * list_title )
{
    return dvd_methods->main_feature(d, list_title);
}

// hb_dvd_set_dvdnav must only be called when no dvd source is open
// it rips the rug out from under things so be careful
void hb_dvd_set_dvdnav( int enable )
{
    if (enable)
        dvd_methods = hb_dvdnav_methods();
    else
        dvd_methods = hb_dvdread_methods();
}

