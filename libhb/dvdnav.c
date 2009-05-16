/* $Id: dvd.c,v 1.12 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "lang.h"
#include "dvd.h"

#include "dvdnav/dvdnav.h"
#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"

#define DVD_READ_CACHE 1

static char        * hb_dvdnav_name( char * path );
static hb_dvd_t    * hb_dvdnav_init( char * path );
static int           hb_dvdnav_title_count( hb_dvd_t * d );
static hb_title_t  * hb_dvdnav_title_scan( hb_dvd_t * d, int t );
static int           hb_dvdnav_start( hb_dvd_t * d, hb_title_t *title, int chapter );
static void          hb_dvdnav_stop( hb_dvd_t * d );
static int           hb_dvdnav_seek( hb_dvd_t * d, float f );
static int           hb_dvdnav_read( hb_dvd_t * d, hb_buffer_t * b );
static int           hb_dvdnav_chapter( hb_dvd_t * d );
static void          hb_dvdnav_close( hb_dvd_t ** _d );
static int           hb_dvdnav_angle_count( hb_dvd_t * d );
static void          hb_dvdnav_set_angle( hb_dvd_t * e, int angle );

hb_dvd_func_t hb_dvdnav_func =
{
    hb_dvdnav_init,
    hb_dvdnav_close,
    hb_dvdnav_name,
    hb_dvdnav_title_count,
    hb_dvdnav_title_scan,
    hb_dvdnav_start,
    hb_dvdnav_stop,
    hb_dvdnav_seek,
    hb_dvdnav_read,
    hb_dvdnav_chapter,
    hb_dvdnav_angle_count,
    hb_dvdnav_set_angle
};

// there can be at most 999 PGCs per title. round that up to the nearest
// power of two.
#define MAX_PGCN 1024

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void PgcWalkInit( uint32_t pgcn_map[MAX_PGCN/32] );
static int FindChapterIndex( hb_list_t * list, int pgcn, int pgn );
static int NextPgcn( ifo_handle_t *ifo, int pgcn, uint32_t pgcn_map[MAX_PGCN/32] );
static int FindNextCell( pgc_t *pgc, int cell_cur );
static int dvdtime2msec( dvd_time_t * );

hb_dvd_func_t * hb_dvdnav_methods( void )
{
    return &hb_dvdnav_func;
}

static char * hb_dvdnav_name( char * path )
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
 * hb_dvdnav_reset
 ***********************************************************************
 * Once dvdnav has entered the 'stopped' state, it can not be revived
 * dvdnav_reset doesn't work because it doesn't remember the path
 * So this function re-opens dvdnav
 **********************************************************************/
static int hb_dvdnav_reset( hb_dvdnav_t * d )
{
    if ( d->dvdnav ) 
        dvdnav_close( d->dvdnav );

    /* Open device */
    if( dvdnav_open(&d->dvdnav, d->path) != DVDNAV_STATUS_OK )
    {
        /*
         * Not an error, may be a stream - which we'll try in a moment.
         */
        hb_log( "dvd: not a dvd - trying as a stream/file instead" );
        goto fail;
    }

    if (dvdnav_set_readahead_flag(d->dvdnav, DVD_READ_CACHE) !=
        DVDNAV_STATUS_OK)
    {
        hb_error("Error: dvdnav_set_readahead_flag: %s\n",
                 dvdnav_err_to_string(d->dvdnav));
        goto fail;
    }

    /*
     ** set the PGC positioning flag to have position information
     ** relatively to the whole feature instead of just relatively to the
     ** current chapter 
     **/
    if (dvdnav_set_PGC_positioning_flag(d->dvdnav, 1) != DVDNAV_STATUS_OK)
    {
        hb_error("Error: dvdnav_set_PGC_positioning_flag: %s\n",
                 dvdnav_err_to_string(d->dvdnav));
        goto fail;
    }
    return 1;

fail:
    if( d->dvdnav ) dvdnav_close( d->dvdnav );
    return 0;
}

/***********************************************************************
 * hb_dvdnav_init
 ***********************************************************************
 *
 **********************************************************************/
static hb_dvd_t * hb_dvdnav_init( char * path )
{
    hb_dvd_t * e;
    hb_dvdnav_t * d;

    e = calloc( sizeof( hb_dvd_t ), 1 );
    d = &(e->dvdnav);

    /* Open device */
    if( dvdnav_open(&d->dvdnav, path) != DVDNAV_STATUS_OK )
    {
        /*
         * Not an error, may be a stream - which we'll try in a moment.
         */
        hb_log( "dvd: not a dvd - trying as a stream/file instead" );
        goto fail;
    }

    if (dvdnav_set_readahead_flag(d->dvdnav, DVD_READ_CACHE) !=
        DVDNAV_STATUS_OK)
    {
        hb_error("Error: dvdnav_set_readahead_flag: %s\n",
                 dvdnav_err_to_string(d->dvdnav));
        goto fail;
    }

    /*
     ** set the PGC positioning flag to have position information
     ** relatively to the whole feature instead of just relatively to the
     ** current chapter 
     **/
    if (dvdnav_set_PGC_positioning_flag(d->dvdnav, 1) != DVDNAV_STATUS_OK)
    {
        hb_error("Error: dvdnav_set_PGC_positioning_flag: %s\n",
                 dvdnav_err_to_string(d->dvdnav));
        goto fail;
    }

    /* Open device */
    if( !( d->reader = DVDOpen( path ) ) )
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
        hb_error( "dvd: ifoOpen failed" );
        goto fail;
    }

    d->path = strdup( path );

    return e;

fail:
    if( d->dvdnav ) dvdnav_close( d->dvdnav );
    if( d->vmg )    ifoClose( d->vmg );
    if( d->reader ) DVDClose( d->reader );
    free( e );
    return NULL;
}

/***********************************************************************
 * hb_dvdnav_title_count
 **********************************************************************/
static int hb_dvdnav_title_count( hb_dvd_t * e )
{
    int titles = 0;
    hb_dvdnav_t * d = &(e->dvdnav);

    dvdnav_get_number_of_titles(d->dvdnav, &titles);
    return titles;
}

static uint64_t
PttDuration(ifo_handle_t *ifo, int ttn, int pttn, int *blocks, int *last_pgcn)
{
    int            pgcn, pgn;
    pgc_t        * pgc;
    uint64_t       duration = 0;
    int            cell_start, cell_end;
    int            i;

    // Initialize map of visited pgc's to prevent loops
    uint32_t pgcn_map[MAX_PGCN/32];
    PgcWalkInit( pgcn_map );
    pgcn   = ifo->vts_ptt_srpt->title[ttn-1].ptt[pttn-1].pgcn;
    pgn   = ifo->vts_ptt_srpt->title[ttn-1].ptt[pttn-1].pgn;
    if ( pgcn < 1 || pgcn > ifo->vts_pgcit->nr_of_pgci_srp || pgcn >= MAX_PGCN)
    {
        hb_error( "invalid PGC ID %d, skipping", pgcn );
        return 0;
    }

    if( pgn <= 0 || pgn > 99 )
    {
        hb_error( "scan: pgn %d not valid, skipping", pgn );
        return 0;
    }

    *blocks = 0;
    do
    {
        pgc = ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;
        if (!pgc)
        {
            hb_error( "scan: pgc not valid, skipping" );
            break;
        }
        duration += 90LL * dvdtime2msec( &pgc->playback_time );

        cell_start = pgc->program_map[pgn-1] - 1;
        cell_end = pgc->nr_of_cells - 1;
        for(i = cell_start; i <= cell_end; i = FindNextCell(pgc, i))
        {
            *blocks += pgc->cell_playback[i].last_sector + 1 -
                pgc->cell_playback[i].first_sector;
        }
        *last_pgcn = pgcn;
        pgn = 1;
    } while((pgcn = NextPgcn(ifo, pgcn, pgcn_map)) != 0);
    return duration;
}

/***********************************************************************
 * hb_dvdnav_title_scan
 **********************************************************************/
static hb_title_t * hb_dvdnav_title_scan( hb_dvd_t * e, int t )
{

    hb_dvdnav_t * d = &(e->dvdnav);
    hb_title_t   * title;
    ifo_handle_t * ifo = NULL;
    int            pgcn, pgn, pgcn_end, i, c;
    int            title_pgcn;
    pgc_t        * pgc;
    int            cell_cur;
    hb_chapter_t * chapter;
    int            count;
    uint64_t       duration, longest;
    int            longest_pgcn, longest_pgn, longest_pgcn_end;
    float          duration_correction;
    const char   * name;

    hb_log( "scan: scanning title %d", t );

    title = hb_title_init( d->path, t );
    if (dvdnav_get_title_string(d->dvdnav, &name) == DVDNAV_STATUS_OK)
    {
        strncpy( title->name, name, sizeof( title->name ) );
    }
    else
    {
        char * p_cur, * p_last = d->path;
        for( p_cur = d->path; *p_cur; p_cur++ )
        {
            if( p_cur[0] == '/' && p_cur[1] )
            {
                p_last = &p_cur[1];
            }
        }
        snprintf( title->name, sizeof( title->name ), "%s", p_last );
    }

    /* VTS which our title is in */
    title->vts = d->vmg->tt_srpt->title[t-1].title_set_nr;

    if ( !title->vts )
    {
        /* A VTS of 0 means the title wasn't found in the title set */
        hb_error("Invalid VTS (title set) number: %i", title->vts);
        goto fail;
    }

    hb_log( "scan: opening IFO for VTS %d", title->vts );
    if( !( ifo = ifoOpen( d->reader, title->vts ) ) )
    {
        hb_error( "scan: ifoOpen failed" );
        goto fail;
    }

    /* ignore titles with bogus cell addresses so we don't abort later
     ** in libdvdread. */
    for ( i = 0; i < ifo->vts_c_adt->nr_of_vobs; ++i)
    {
        if( (ifo->vts_c_adt->cell_adr_table[i].start_sector & 0xffffff ) ==
            0xffffff )
        {
            hb_error( "scan: cell_adr_table[%d].start_sector invalid (0x%x) "
                      "- skipping title", i,
                      ifo->vts_c_adt->cell_adr_table[i].start_sector );
            goto fail;
        }
        if( (ifo->vts_c_adt->cell_adr_table[i].last_sector & 0xffffff ) ==
            0xffffff )
        {
            hb_error( "scan: cell_adr_table[%d].last_sector invalid (0x%x) "
                      "- skipping title", i,
                      ifo->vts_c_adt->cell_adr_table[i].last_sector );
            goto fail;
        }
        if( ifo->vts_c_adt->cell_adr_table[i].start_sector >=
            ifo->vts_c_adt->cell_adr_table[i].last_sector )
        {
            hb_error( "scan: cell_adr_table[%d].start_sector (0x%x) "
                      "is not before last_sector (0x%x) - skipping title", i,
                      ifo->vts_c_adt->cell_adr_table[i].start_sector,
                      ifo->vts_c_adt->cell_adr_table[i].last_sector );
            goto fail;
        }
    }

    if( global_verbosity_level == 3 )
    {
        ifo_print( d->reader, title->vts );
    }

    /* Position of the title in the VTS */
    title->ttn = d->vmg->tt_srpt->title[t-1].vts_ttn;
    if ( title->ttn < 1 || title->ttn > ifo->vts_ptt_srpt->nr_of_srpts )
    {
        hb_error( "invalid VTS PTT offset %d for title %d, skipping", title->ttn, t );
        goto fail;
    }

    longest = 0LL;
    longest_pgcn = -1;
    longest_pgn = 1;
    longest_pgcn_end = -1;
    pgcn_end = -1;
    for( i = 0; i < ifo->vts_ptt_srpt->title[title->ttn-1].nr_of_ptts; i++ )
    {
        int blocks = 0;

        duration = PttDuration(ifo, title->ttn, i+1, &blocks, &pgcn_end);
        pgcn  = ifo->vts_ptt_srpt->title[title->ttn-1].ptt[i].pgcn;
        pgn   = ifo->vts_ptt_srpt->title[title->ttn-1].ptt[i].pgn;
        if( duration > longest )
        {
            longest_pgcn  = pgcn;
            longest_pgn   = pgn;
            longest_pgcn_end   = pgcn_end;
            longest = duration;
            title->block_count = blocks;
        }
        else if (pgcn == longest_pgcn && pgn < longest_pgn)
        {
            longest_pgn   = pgn;
            title->block_count = blocks;
        }
    }

    /* ignore titles under 10 seconds because they're often stills or
     * clips with no audio & our preview code doesn't currently handle
     * either of these. */
    if( longest < 900000LL )
    {
        hb_log( "scan: ignoring title (too short)" );
        goto fail;
    }

    pgcn       = longest_pgcn;
    pgcn_end   = longest_pgcn_end;
    pgn        = longest_pgn;;
    title_pgcn = pgcn;


    /* Get pgc */
    if ( pgcn < 1 || pgcn > ifo->vts_pgcit->nr_of_pgci_srp || pgcn >= MAX_PGCN)
    {
        hb_error( "invalid PGC ID %d for title %d, skipping", pgcn, t );
        goto fail;
    }

    pgc = ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;

    hb_log("pgc_id: %d, pgn: %d: pgc: 0x%x", pgcn, pgn, pgc);

    /* Title start */
    title->cell_start = pgc->program_map[pgn-1] - 1;
    title->block_start = pgc->cell_playback[title->cell_start].first_sector;

    pgc = ifo->vts_pgcit->pgci_srp[pgcn_end-1].pgc;

    /* Title end */
    title->cell_end = pgc->nr_of_cells - 1;
    title->block_end = pgc->cell_playback[title->cell_end].last_sector;

    hb_log( "scan: vts=%d, ttn=%d, cells=%d->%d, blocks=%d->%d, "
            "%d blocks", title->vts, title->ttn, title->cell_start,
            title->cell_end, title->block_start, title->block_end,
            title->block_count );

    /* Get duration */
    title->duration = longest;
    title->hours    = title->duration / 90000 / 3600;
    title->minutes  = ( ( title->duration / 90000 ) % 3600 ) / 60;
    title->seconds  = ( title->duration / 90000 ) % 60;
    hb_log( "scan: duration is %02d:%02d:%02d (%lld ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* Detect languages */
    for( i = 0; i < ifo->vtsi_mat->nr_of_vts_audio_streams; i++ )
    {
        hb_audio_t * audio, * audio_tmp;
        int          audio_format, lang_code, audio_control,
                     position, j;
        iso639_lang_t * lang;
        int lang_extension = 0;

        hb_log( "scan: checking audio %d", i + 1 );

        audio = calloc( sizeof( hb_audio_t ), 1 );

        audio_format  = ifo->vtsi_mat->vts_audio_attr[i].audio_format;
        lang_code     = ifo->vtsi_mat->vts_audio_attr[i].lang_code;
        lang_extension = ifo->vtsi_mat->vts_audio_attr[i].code_extension;
        audio_control =
            ifo->vts_pgcit->pgci_srp[title_pgcn-1].pgc->audio_control[i];

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
                break;

            case 0x02:
            case 0x03:
                audio->id    = 0xc0 + position;
                audio->config.in.codec = HB_ACODEC_MPGA;
                break;

            case 0x04:
                audio->id    = ( ( 0xa0 + position ) << 8 ) | 0xbd;
                audio->config.in.codec = HB_ACODEC_LPCM;
                break;

            case 0x06:
                audio->id    = ( ( 0x88 + position ) << 8 ) | 0xbd;
                audio->config.in.codec = HB_ACODEC_DCA;
                break;

            default:
                audio->id    = 0;
                audio->config.in.codec = 0;
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

        audio->config.lang.type = lang_extension;

        lang = lang_for_code( ifo->vtsi_mat->vts_audio_attr[i].lang_code );

        snprintf( audio->config.lang.description, sizeof( audio->config.lang.description ), "%s (%s)",
            strlen(lang->native_name) ? lang->native_name : lang->eng_name,
            audio->config.in.codec == HB_ACODEC_AC3 ? "AC3" : ( audio->config.in.codec ==
                HB_ACODEC_DCA ? "DTS" : ( audio->config.in.codec ==
                HB_ACODEC_MPGA ? "MPEG" : "LPCM" ) ) );
        snprintf( audio->config.lang.simple, sizeof( audio->config.lang.simple ), "%s",
                  strlen(lang->native_name) ? lang->native_name : lang->eng_name );
        snprintf( audio->config.lang.iso639_2, sizeof( audio->config.lang.iso639_2 ), "%s",
                  lang->iso639_2);

        switch( lang_extension )
        {
        case 0:
        case 1:
            break;
        case 2:
            strcat( audio->config.lang.description, " (Visually Impaired)" );
            break;
        case 3:
            strcat( audio->config.lang.description, " (Director's Commentary 1)" );
            break;
        case 4:
            strcat( audio->config.lang.description, " (Director's Commentary 2)" );
            break;
        default:
            break;
        }

        hb_log( "scan: id=%x, lang=%s, 3cc=%s ext=%i", audio->id,
                audio->config.lang.description, audio->config.lang.iso639_2,
                lang_extension );

        audio->config.in.track = i;
        hb_list_add( title->list_audio, audio );
    }

    if( !hb_list_count( title->list_audio ) )
    {
        hb_log( "scan: ignoring title (no audio track)" );
        goto fail;
    }

    memcpy( title->palette,
            ifo->vts_pgcit->pgci_srp[title_pgcn-1].pgc->palette,
            16 * sizeof( uint32_t ) );

    /* Check for subtitles */
    for( i = 0; i < ifo->vtsi_mat->nr_of_vts_subp_streams; i++ )
    {
        hb_subtitle_t * subtitle;
        int spu_control;
        int position;
        iso639_lang_t * lang;
        int lang_extension = 0;

        hb_log( "scan: checking subtitle %d", i + 1 );

        spu_control =
            ifo->vts_pgcit->pgci_srp[title_pgcn-1].pgc->subp_control[i];

        if( !( spu_control & 0x80000000 ) )
        {
            hb_log( "scan: subtitle channel is not active" );
            continue;
        }

        if( ifo->vtsi_mat->vts_video_attr.display_aspect_ratio )
        {
            switch( ifo->vtsi_mat->vts_video_attr.permitted_df )
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

        lang_extension = ifo->vtsi_mat->vts_subp_attr[i].code_extension;

        lang = lang_for_code( ifo->vtsi_mat->vts_subp_attr[i].lang_code );

        subtitle = calloc( sizeof( hb_subtitle_t ), 1 );
        subtitle->id = ( ( 0x20 + position ) << 8 ) | 0xbd;
        snprintf( subtitle->lang, sizeof( subtitle->lang ), "%s",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name);
        snprintf( subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "%s",
                  lang->iso639_2);

        subtitle->type = lang_extension;

        switch( lang_extension )
        {  
        case 0:
            break;
        case 1:
            break;
        case 2:
            strcat( subtitle->lang, " (Caption with bigger size character)");
            break;
        case 3: 
            strcat( subtitle->lang, " (Caption for Children)");
            break;
        case 4:
            break;
        case 5:
            strcat( subtitle->lang, " (Closed Caption)");
            break;
        case 6:
            strcat( subtitle->lang, " (Closed Caption with bigger size character)");
            break;
        case 7:
            strcat( subtitle->lang, " (Closed Caption for Children)");
            break;
        case 8:
            break;
        case 9:
            strcat( subtitle->lang, " (Forced Caption)");
            break;
        case 10:
            break;
        case 11:
            break;
        case 12:
            break;
        case 13:
            strcat( subtitle->lang, " (Director's Commentary)");
            break;
        case 14:
            strcat( subtitle->lang, " (Director's Commentary with bigger size character)");
            break;
        case 15:
            strcat( subtitle->lang, " (Director's Commentary for Children)");
        default:
            break;
        }

        hb_log( "scan: id=%x, lang=%s, 3cc=%s", subtitle->id,
                subtitle->lang, subtitle->iso639_2 );

        hb_list_add( title->list_subtitle, subtitle );
    }

    /* Chapters */
    uint32_t pgcn_map[MAX_PGCN/32];
    PgcWalkInit( pgcn_map );
    c = 0;
    do
    {
        pgc = ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;

        for (i = pgn; i <= pgc->nr_of_programs; i++)
        {
            chapter = calloc( sizeof( hb_chapter_t ), 1 );

            chapter->index = c + 1;
            chapter->pgcn = pgcn;
            chapter->pgn = i;
            hb_list_add( title->list_chapter, chapter );
            c++;
        }

        pgn = 1;
    } while ((pgcn = NextPgcn(ifo, pgcn, pgcn_map)) != 0);

    hb_log( "scan: title %d has %d chapters", t, c );

    duration = 0;
    count = hb_list_count( title->list_chapter );
    for (i = 0; i < count; i++)
    {
        chapter = hb_list_item( title->list_chapter, i );

        pgcn = chapter->pgcn;
        pgn = chapter->pgn;
        pgc = ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;

        /* Start cell */
        chapter->cell_start  = pgc->program_map[pgn-1] - 1;
        chapter->block_start = pgc->cell_playback[chapter->cell_start].first_sector;
        // if there are no more programs in this pgc, the end cell is the
        // last cell. Otherwise it's the cell before the start cell of the
        // next program.
        if ( pgn == pgc->nr_of_programs )
        {
            chapter->cell_end = pgc->nr_of_cells - 1;
        }
        else
        {
            chapter->cell_end = pgc->program_map[pgn] - 2;;
        }
        chapter->block_end = pgc->cell_playback[chapter->cell_end].last_sector;

        /* Block count, duration */
        chapter->block_count = 0;
        chapter->duration = 0;

        cell_cur = chapter->cell_start;
        while( cell_cur <= chapter->cell_end )
        {
#define cp pgc->cell_playback[cell_cur]
            chapter->block_count += cp.last_sector + 1 - cp.first_sector;
            chapter->duration += 90LL * dvdtime2msec( &cp.playback_time );
#undef cp
            cell_cur = FindNextCell( pgc, cell_cur );
        }
        duration += chapter->duration;
    }

    /* The durations we get for chapters aren't precise. Scale them so
       the total matches the title duration */
    duration_correction = (float) title->duration / (float) duration;
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        int seconds;
        chapter            = hb_list_item( title->list_chapter, i );
        chapter->duration  = duration_correction * chapter->duration;
        seconds            = ( chapter->duration + 45000 ) / 90000;
        chapter->hours     = seconds / 3600;
        chapter->minutes   = ( seconds % 3600 ) / 60;
        chapter->seconds   = seconds % 60;

        hb_log( "scan: chap %d c=%d->%d, b=%d->%d (%d), %lld ms",
                chapter->index, chapter->cell_start, chapter->cell_end,
                chapter->block_start, chapter->block_end,
                chapter->block_count, chapter->duration / 90 );
    }

    /* Get aspect. We don't get width/height/rate infos here as
       they tend to be wrong */
    switch( ifo->vtsi_mat->vts_video_attr.display_aspect_ratio )
    {
        case 0:
            title->container_aspect = 4. / 3.;
            break;
        case 3:
            title->container_aspect = 16. / 9.;
            break;
        default:
            hb_log( "scan: unknown aspect" );
            goto fail;
    }

    hb_log( "scan: aspect = %g", title->aspect );

    /* This title is ok so far */
    goto cleanup;

fail:
    hb_list_close( &title->list_audio );
    free( title );
    title = NULL;

cleanup:
    if( ifo ) ifoClose( ifo );

    return title;
}

/***********************************************************************
 * hb_dvdnav_start
 ***********************************************************************
 * Title and chapter start at 1
 **********************************************************************/
static int hb_dvdnav_start( hb_dvd_t * e, hb_title_t *title, int c )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    int t = title->index;
    hb_chapter_t *chapter;
    dvdnav_status_t result;

    d->title_block_count = title->block_count;
    d->list_chapter = title->list_chapter;

    if ( d->stopped && !hb_dvdnav_reset(d) )
    {
        return 0;
    }
    chapter = hb_list_item( title->list_chapter, c - 1);
    if (chapter != NULL)
        result = dvdnav_program_play(d->dvdnav, t, chapter->pgcn, chapter->pgn);
    else
        result = dvdnav_part_play(d->dvdnav, t, 1);
    if (result != DVDNAV_STATUS_OK)
    {
        hb_error( "dvd: dvdnav_*_play failed - %s", 
                  dvdnav_err_to_string(d->dvdnav) );
        return 0;
    }
    d->title = t;
    d->stopped = 0;
    return 1;
}

/***********************************************************************
 * hb_dvdnav_stop
 ***********************************************************************
 *
 **********************************************************************/
static void hb_dvdnav_stop( hb_dvd_t * e )
{
}

/***********************************************************************
 * hb_dvdnav_seek
 ***********************************************************************
 *
 **********************************************************************/
static int hb_dvdnav_seek( hb_dvd_t * e, float f )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    uint64_t sector = f * d->title_block_count;
    int result, event, len;
    uint8_t buf[HB_DVD_READ_BUFFER_SIZE];
    int done = 0, ii;

    if (d->stopped)
    {
        return 0;
    }

    // XXX the current version of libdvdnav can't seek outside the current
    // PGC. Check if the place we're seeking to is in a different
    // PGC. Position there & adjust the offset if so.
    hb_chapter_t *pgc_change = hb_list_item(d->list_chapter, 0 );
    for ( ii = 0; ii < hb_list_count( d->list_chapter ); ++ii )
    {
        hb_chapter_t *chapter = hb_list_item( d->list_chapter, ii );

        if ( chapter->pgcn != pgc_change->pgcn )
        {
            // this chapter's in a different pgc from the previous - note the
            // change so we can make sector offset's be pgc relative.
            pgc_change = chapter;
        }
        if ( chapter->block_start <= sector && sector <= chapter->block_end )
        {
            // this chapter contains the sector we want - see if it's in a
            // different pgc than the one we're currently in.
            int32_t title, pgcn, pgn;
            if (dvdnav_current_title_program( d->dvdnav, &title, &pgcn, &pgn ) != DVDNAV_STATUS_OK)
                hb_log("dvdnav cur pgcn err: %s", dvdnav_err_to_string(d->dvdnav));
            if ( d->title != title || chapter->pgcn != pgcn )
            {
                // this chapter is in a different pgc - switch to it.
                if (dvdnav_program_play(d->dvdnav, d->title, chapter->pgcn, chapter->pgn) != DVDNAV_STATUS_OK)
                    hb_log("dvdnav prog play err: %s", dvdnav_err_to_string(d->dvdnav));
            }
            // seek sectors are pgc-relative so remove the pgc start sector.
            sector -= pgc_change->block_start;
            break;
        }
    }

    // dvdnav will not let you seek or poll current position
    // till it reaches a certain point in parsing.  so we
    // have to get blocks until we reach a cell
    // Put an arbitrary limit of 100 blocks on how long we search
    for (ii = 0; ii < 100 && !done; ii++)
    {
        result = dvdnav_get_next_block( d->dvdnav, buf, &event, &len );
        if ( result == DVDNAV_STATUS_ERR )
        {
            hb_log("dvdnav: Read Error, %s", dvdnav_err_to_string(d->dvdnav));
            return 0;
        }
        switch ( event )
        {
        case DVDNAV_BLOCK_OK:
        case DVDNAV_CELL_CHANGE:
            done = 1;
            break;

        case DVDNAV_STILL_FRAME:
            dvdnav_still_skip( d->dvdnav );
            break;

        case DVDNAV_WAIT:
            dvdnav_wait_skip( d->dvdnav );
            break;

        case DVDNAV_STOP:
            hb_log("dvdnav: stop encountered during seek");
            d->stopped = 1;
            return 0;

        case DVDNAV_HOP_CHANNEL:
        case DVDNAV_NAV_PACKET:
        case DVDNAV_VTS_CHANGE:
        case DVDNAV_HIGHLIGHT:
        case DVDNAV_AUDIO_STREAM_CHANGE:
        case DVDNAV_SPU_STREAM_CHANGE:
        case DVDNAV_SPU_CLUT_CHANGE:
        case DVDNAV_NOP:
        default:
            break;
        }
    }

    if (dvdnav_sector_search(d->dvdnav, sector, SEEK_SET) != DVDNAV_STATUS_OK)
    {
        hb_error( "dvd: dvdnav_sector_search failed - %s", 
                  dvdnav_err_to_string(d->dvdnav) );
        return 0;
    }
    return 1;
}

/***********************************************************************
 * hb_dvdnav_read
 ***********************************************************************
 *
 **********************************************************************/
static int hb_dvdnav_read( hb_dvd_t * e, hb_buffer_t * b )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    int result, event, len;
    int chapter = 0;

    while ( 1 )
    {
        if (d->stopped)
        {
            return 0;
        }
        result = dvdnav_get_next_block( d->dvdnav, b->data, &event, &len );
        if ( result == DVDNAV_STATUS_ERR )
        {
            hb_log("dvdnav: Read Error, %s", dvdnav_err_to_string(d->dvdnav));
            return 0;
        }
        switch ( event )
        {
        case DVDNAV_BLOCK_OK:
            // We have received a regular block of the currently playing
            // MPEG stream.

            // The muxers expect to only get chapter 2 and above
            // They write chapter 1 when chapter 2 is detected.
            if (chapter > 1)
                b->new_chap = chapter;
            chapter = 0;
            return 1;

        case DVDNAV_NOP:
            /*
            * Nothing to do here. 
            */
            break;

        case DVDNAV_STILL_FRAME:
            /*
            * We have reached a still frame. A real player application
            * would wait the amount of time specified by the still's
            * length while still handling user input to make menus and
            * other interactive stills work. A length of 0xff means an
            * indefinite still which has to be skipped indirectly by some 
            * user interaction. 
            */
            dvdnav_still_skip( d->dvdnav );
            break;

        case DVDNAV_WAIT:
            /*
            * We have reached a point in DVD playback, where timing is
            * critical. Player application with internal fifos can
            * introduce state inconsistencies, because libdvdnav is
            * always the fifo's length ahead in the stream compared to
            * what the application sees. Such applications should wait
            * until their fifos are empty when they receive this type of
            * event. 
            */
            dvdnav_wait_skip( d->dvdnav );
            break;

        case DVDNAV_SPU_CLUT_CHANGE:
            /*
            * Player applications should pass the new colour lookup table 
            * to their SPU decoder 
            */
            break;

        case DVDNAV_SPU_STREAM_CHANGE:
            /*
            * Player applications should inform their SPU decoder to
            * switch channels 
            */
            break;

        case DVDNAV_AUDIO_STREAM_CHANGE:
            /*
            * Player applications should inform their audio decoder to
            * switch channels 
            */
            break;

        case DVDNAV_HIGHLIGHT:
            /*
            * Player applications should inform their overlay engine to
            * highlight the given button 
            */
            break;

        case DVDNAV_VTS_CHANGE:
            /*
            * Some status information like video aspect and video scale
            * permissions do not change inside a VTS. Therefore this
            * event can be used to query such information only when
            * necessary and update the decoding/displaying accordingly. 
            */
            break;

        case DVDNAV_CELL_CHANGE:
            /*
            * Some status information like the current Title and Part
            * numbers do not change inside a cell. Therefore this event
            * can be used to query such information only when necessary
            * and update the decoding/displaying accordingly. 
            */
            {
                int tt = 0, pgcn = 0, pgn = 0, c;

                dvdnav_current_title_program(d->dvdnav, &tt, &pgcn, &pgn);
                if (tt != d->title)
                {
                    // Transition to another title signals that we are done.
                    return 0;
                }
                c = FindChapterIndex(d->list_chapter, pgcn, pgn);
                if (c > d->chapter)
                    chapter = d->chapter = c;
            }
            break;

        case DVDNAV_NAV_PACKET:
            /*
            * A NAV packet provides PTS discontinuity information, angle
            * linking information and button definitions for DVD menus.
            * Angles are handled completely inside libdvdnav. For the
            * menus to work, the NAV packet information has to be passed
            * to the overlay engine of the player so that it knows the
            * dimensions of the button areas. 
            */

            // mpegdemux expects to get these.  I don't think it does
            // anything useful with them however.

            // The muxers expect to only get chapter 2 and above
            // They write chapter 1 when chapter 2 is detected.
            if (chapter > 1)
                b->new_chap = chapter;
            chapter = 0;
            return 1;

            break;

        case DVDNAV_HOP_CHANNEL:
            /*
            * This event is issued whenever a non-seamless operation has
            * been executed. Applications with fifos should drop the
            * fifos content to speed up responsiveness. 
            */
            break;

        case DVDNAV_STOP:
            /*
            * Playback should end here. 
            */
            d->stopped = 1;
            return 0;

        default:
            break;
        }
    }
    return 0;
}

/***********************************************************************
 * hb_dvdnav_chapter
 ***********************************************************************
 * Returns in which chapter the next block to be read is.
 * Chapter numbers start at 1.
 **********************************************************************/
static int hb_dvdnav_chapter( hb_dvd_t * e )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    int32_t t, pgcn, pgn;
    int32_t c;

    if (dvdnav_current_title_program(d->dvdnav, &t, &pgcn, &pgn) != DVDNAV_STATUS_OK)
    {
        return -1;
    }
    c = FindChapterIndex( d->list_chapter, pgcn, pgn );
    return c;
}

/***********************************************************************
 * hb_dvdnav_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
static void hb_dvdnav_close( hb_dvd_t ** _d )
{
    hb_dvdnav_t * d = &((*_d)->dvdnav);

    if( d->dvdnav ) dvdnav_close( d->dvdnav );
    if( d->vmg ) ifoClose( d->vmg );
    if( d->reader ) DVDClose( d->reader );

    free( d );
    *_d = NULL;
}

/***********************************************************************
 * hb_dvdnav_angle_count
 ***********************************************************************
 * Returns the number of angles supported.
 **********************************************************************/
static int hb_dvdnav_angle_count( hb_dvd_t * e )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    int current, angle_count;

    if (dvdnav_get_angle_info( d->dvdnav, &current, &angle_count) != DVDNAV_STATUS_OK)
    {
        hb_log("dvdnav_get_angle_info %s", dvdnav_err_to_string(d->dvdnav));
        angle_count = 1;
    }
    return angle_count;
}

/***********************************************************************
 * hb_dvdnav_set_angle
 ***********************************************************************
 * Sets the angle to read
 **********************************************************************/
static void hb_dvdnav_set_angle( hb_dvd_t * e, int angle )
{
    hb_dvdnav_t * d = &(e->dvdnav);

    if (dvdnav_angle_change( d->dvdnav, angle) != DVDNAV_STATUS_OK)
    {
        hb_log("dvdnav_angle_change %s", dvdnav_err_to_string(d->dvdnav));
    }
}

/***********************************************************************
 * FindChapterIndex
 ***********************************************************************
 * Assumes pgc and cell_cur are correctly set, and sets cell_next to the
 * cell to be read when we will be done with cell_cur.
 **********************************************************************/
static int FindChapterIndex( hb_list_t * list, int pgcn, int pgn )
{
    int count, ii;
    hb_chapter_t *chapter;

    count = hb_list_count( list );
    for (ii = 0; ii < count; ii++)
    {
        chapter = hb_list_item( list, ii );
        if (chapter->pgcn == pgcn && chapter->pgn == pgn)
            return chapter->index;
    }
    return 0;
}

/***********************************************************************
 * FindNextCell
 ***********************************************************************
 * Assumes pgc and cell_cur are correctly set, and sets cell_next to the
 * cell to be read when we will be done with cell_cur.
 **********************************************************************/
static int FindNextCell( pgc_t *pgc, int cell_cur )
{
    int i = 0;
    int cell_next;

    if( pgc->cell_playback[cell_cur].block_type ==
            BLOCK_TYPE_ANGLE_BLOCK )
    {

        while( pgc->cell_playback[cell_cur+i].block_mode !=
                   BLOCK_MODE_LAST_CELL )
        {
             i++;
        }
        cell_next = cell_cur + i + 1;
        hb_log( "dvd: Skipping multi-angle cells %d-%d",
                cell_cur,
                cell_next - 1 );
    }
    else
    {
        cell_next = cell_cur + 1;
    }
    return cell_next;
}

/***********************************************************************
 * NextPgcn
 ***********************************************************************
 * Assumes pgc and cell_cur are correctly set, and sets cell_next to the
 * cell to be read when we will be done with cell_cur.
 * Since pg chains can be circularly linked (either from a read error or
 * deliberately) pgcn_map tracks program chains we've already seen.
 **********************************************************************/
static int NextPgcn( ifo_handle_t *ifo, int pgcn, uint32_t pgcn_map[MAX_PGCN/32] )
{
    int next_pgcn;
    pgc_t *pgc;

    pgcn_map[pgcn >> 5] |= (1 << (pgcn & 31));

    pgc = ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;
    next_pgcn = pgc->next_pgc_nr;
    if ( next_pgcn < 1 || next_pgcn >= MAX_PGCN || next_pgcn > ifo->vts_pgcit->nr_of_pgci_srp )
        return 0;

    return pgcn_map[next_pgcn >> 5] & (1 << (next_pgcn & 31))? 0 : next_pgcn;
}

/***********************************************************************
 * PgcWalkInit
 ***********************************************************************
 * Pgc links can loop. I track which have been visited in a bit vector
 * Initialize the bit vector to empty.
 **********************************************************************/
static void PgcWalkInit( uint32_t pgcn_map[MAX_PGCN/32] )
{
    memset(pgcn_map, 0, sizeof(pgcn_map) );
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
        ms += ((dt->frame_u & 0x30) >> 3) * 5 +
              (dt->frame_u & 0x0f) * 1000.0 / fps;
    }

    return ms;
}
