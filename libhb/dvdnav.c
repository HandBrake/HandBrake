/* dvdnav.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "libavcodec/avcodec.h"

#include "handbrake/handbrake.h"
#include "handbrake/lang.h"
#include "handbrake/dvd.h"

#include "dvdnav/dvdnav.h"
#include "dvdread/ifo_read.h"
#include "dvdread/ifo_print.h"
#include "dvdread/nav_read.h"

#define DVD_READ_CACHE 1

static char        * hb_dvdnav_name( char * path );
static hb_dvd_t    * hb_dvdnav_init( hb_handle_t * h, const char * path );
static int           hb_dvdnav_title_count( hb_dvd_t * d );
static hb_title_t  * hb_dvdnav_title_scan( hb_dvd_t * d, int t, uint64_t min_duration, uint64_t max_duration );
static int           hb_dvdnav_start( hb_dvd_t * d, hb_title_t *title, int chapter );
static void          hb_dvdnav_stop( hb_dvd_t * d );
static int           hb_dvdnav_seek( hb_dvd_t * d, float f );
static hb_buffer_t * hb_dvdnav_read( hb_dvd_t * d );
static int           hb_dvdnav_chapter( hb_dvd_t * d );
static void          hb_dvdnav_close( hb_dvd_t ** _d );
static int           hb_dvdnav_angle_count( hb_dvd_t * d );
static void          hb_dvdnav_set_angle( hb_dvd_t * d, int angle );
static int           hb_dvdnav_main_feature( hb_dvd_t * d, hb_list_t * list_title );

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
    hb_dvdnav_set_angle,
    hb_dvdnav_main_feature
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
static int TitleOpenIfo(hb_dvdnav_t * d, int t);
static void TitleCloseIfo(hb_dvdnav_t * d);

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
static hb_dvd_t * hb_dvdnav_init( hb_handle_t * h, const char * path )
{
    hb_dvd_t * e;
    hb_dvdnav_t * d;
    int region_mask;

    e = calloc( sizeof( hb_dvd_t ), 1 );
    d = &(e->dvdnav);
    d->h = h;

	/* Log DVD drive region code */
    if ( hb_dvd_region( path, &region_mask ) == 0 )
    {
        hb_log( "dvd: Region mask 0x%02x", region_mask );
        if ( region_mask == 0xFF )
        {
            hb_log( "dvd: Warning, DVD device has no region set" );
        }
    }

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

    *blocks = 0;

    // Initialize map of visited pgc's to prevent loops
    uint32_t pgcn_map[MAX_PGCN/32];
    PgcWalkInit( pgcn_map );
    pgcn   = ifo->vts_ptt_srpt->title[ttn-1].ptt[pttn-1].pgcn;
    pgn   = ifo->vts_ptt_srpt->title[ttn-1].ptt[pttn-1].pgn;
    if ( pgcn < 1 || pgcn > ifo->vts_pgcit->nr_of_pgci_srp || pgcn >= MAX_PGCN)
    {
        hb_log( "invalid PGC ID %d, skipping", pgcn );
        return 0;
    }

    if( pgn <= 0 || pgn > 99 )
    {
        hb_log( "scan: pgn %d not valid, skipping", pgn );
        return 0;
    }

    do
    {
        pgc = ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;
        if (!pgc)
        {
            *blocks = 0;
            duration = 0;
            hb_log( "scan: pgc not valid, skipping" );
            break;
        }

        if (pgc->cell_playback == NULL)
        {
            *blocks = 0;
            duration = 0;
            hb_log("invalid PGC cell_playback table, skipping");
            break;
        }

        if (pgn > pgc->nr_of_programs)
        {
            pgn = 1;
            continue;
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

static void add_subtitle( hb_list_t * list_subtitle, int position,
                          iso639_lang_t * lang, int lang_extension,
                          uint8_t * palette, int style )
{
    hb_subtitle_t * subtitle;
    int ii, count;

    count = hb_list_count(list_subtitle);
    for (ii = 0; ii < count; ii++)
    {
        subtitle = hb_list_item(list_subtitle, ii);
        if (((subtitle->id >> 8) & 0x1f) == position)
        {
            // The subtitle is already in the list
            return;
        }
    }

    subtitle        = calloc(sizeof(hb_subtitle_t), 1);
    subtitle->track = count;
    subtitle->id    = ((0x20 + position) << 8) | 0xbd;
    snprintf(subtitle->lang, sizeof( subtitle->lang ), "%s",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name);
    snprintf(subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "%s",
             lang->iso639_2);
    subtitle->format         = PICTURESUB;
    subtitle->source         = VOBSUB;
    subtitle->config.dest    = RENDERSUB;
    subtitle->stream_type    = 0xbd;
    subtitle->substream_type = 0x20 + position;
    subtitle->codec          = WORK_DECAVSUB;
    subtitle->codec_param    = AV_CODEC_ID_DVD_SUBTITLE;
    subtitle->timebase.num   = 1;
    subtitle->timebase.den   = 90000;

    memcpy(subtitle->palette, palette, 16 * sizeof(uint32_t));
    subtitle->palette_set = 1;

    const char * name = NULL;
    switch (lang_extension)
    {
        case 1:
            subtitle->attributes = HB_SUBTITLE_ATTR_NORMAL;
            break;
        case 2:
            subtitle->attributes = HB_SUBTITLE_ATTR_LARGE;
            strcat(subtitle->lang, " Large Type");
            name = "Large Type";
            break;
        case 3:
            subtitle->attributes = HB_SUBTITLE_ATTR_CHILDREN;
            strcat(subtitle->lang, " Children");
            name = "Children";
            break;
        case 5:
            subtitle->attributes = HB_SUBTITLE_ATTR_CC;
            strcat(subtitle->lang, " Closed Caption");
            name = "Closed Caption";
            break;
        case 6:
            subtitle->attributes = HB_SUBTITLE_ATTR_CC | HB_SUBTITLE_ATTR_LARGE;
            strcat(subtitle->lang, " Closed Caption, Large Type");
            name = "Closed Caption, Large Type";
            break;
        case 7:
            subtitle->attributes = HB_SUBTITLE_ATTR_CC |
                                   HB_SUBTITLE_ATTR_CHILDREN;
            strcat(subtitle->lang, " Closed Caption, Children");
            name = "Closed Caption, Children";
            break;
        case 9:
            subtitle->attributes = HB_SUBTITLE_ATTR_FORCED;
            strcat(subtitle->lang, " Forced");
            break;
        case 13:
            subtitle->attributes = HB_SUBTITLE_ATTR_COMMENTARY;
            strcat(subtitle->lang, " Director's Commentary");
            name = "Commentary";
            break;
        case 14:
            subtitle->attributes = HB_SUBTITLE_ATTR_COMMENTARY |
                                   HB_SUBTITLE_ATTR_LARGE;
            strcat(subtitle->lang, " Director's Commentary, Large Type");
            name = "Commentary, Large Type";
            break;
        case 15:
            subtitle->attributes = HB_SUBTITLE_ATTR_COMMENTARY |
                                   HB_SUBTITLE_ATTR_CHILDREN;
            strcat(subtitle->lang, " Director's Commentary, Children");
            name = "Commentary, Children";
        default:
            subtitle->attributes = HB_SUBTITLE_ATTR_UNKNOWN;
            break;
    }
    if (name != NULL)
    {
        subtitle->name = strdup(name);
    }
    switch (style)
    {
        case HB_VOBSUB_STYLE_4_3:
            subtitle->attributes |= HB_SUBTITLE_ATTR_4_3;
            strcat(subtitle->lang, " (4:3)");
            break;
        case HB_VOBSUB_STYLE_WIDE:
            subtitle->attributes |= HB_SUBTITLE_ATTR_WIDE;
            strcat(subtitle->lang, " (Wide Screen)");
            break;
        case HB_VOBSUB_STYLE_LETTERBOX:
            subtitle->attributes |= HB_SUBTITLE_ATTR_LETTERBOX;
            strcat(subtitle->lang, " (Letterbox)");
            break;
        case HB_VOBSUB_STYLE_PANSCAN:
            subtitle->attributes |= HB_SUBTITLE_ATTR_PANSCAN;
            strcat(subtitle->lang, " (Pan & Scan)");
            break;
    }
    strcat(subtitle->lang, " [");
    strcat(subtitle->lang, hb_subsource_name(subtitle->source));
    strcat(subtitle->lang, "]");

    hb_log("scan: id=0x%x, lang=%s, 3cc=%s ext=%i", subtitle->id,
           subtitle->lang, subtitle->iso639_2, lang_extension);

    hb_list_add(list_subtitle, subtitle);
}

/***********************************************************************
 * hb_dvdnav_title_scan
 **********************************************************************/
static hb_title_t * hb_dvdnav_title_scan( hb_dvd_t * e, int t, uint64_t min_duration, uint64_t max_duration )
{

    hb_dvdnav_t      * d = &(e->dvdnav);
    hb_title_t       * title;
    int                pgcn, i;
    pgc_t            * pgc;
    hb_chapter_t     * chapter;
    hb_dvd_chapter_t * dvd_chapter;
    int                count;
    const char       * title_string;
    char               name[1024];
    unsigned char      unused[1024];
    const char       * codec_name;

    hb_log( "scan: scanning title %d", t );

    title = hb_title_init( d->path, t );
    title->type = HB_DVD_TYPE;
    if (dvdnav_get_title_string(d->dvdnav, &title_string) == DVDNAV_STATUS_OK)
    {
        title->name = strdup(title_string);
    }

    if (title->name == NULL || title->name[0] == 0)
    {
        free((char*)title->name);
        if (DVDUDFVolumeInfo(d->reader, name, sizeof(name),
                             unused, sizeof(unused)))
        {
            char * p_cur, * p_last = d->path;
            for( p_cur = d->path; *p_cur; p_cur++ )
            {
                if( IS_DIR_SEP(p_cur[0]) && p_cur[1] )
                {
                    p_last = &p_cur[1];
                }
            }
            title->name = strdup(p_last);
            char *dot_term = strrchr(title->name, '.');
            if (dot_term)
                *dot_term = '\0';
        }
        else
        {
            title->name = strdup(name);
        }
    }

    if (!TitleOpenIfo(d, t))
    {
        goto fail;
    }

    /* ignore titles with bogus cell addresses so we don't abort later
     ** in libdvdread. */
    for ( i = 0; i < d->ifo->vts_c_adt->nr_of_vobs; ++i)
    {
        if( (d->ifo->vts_c_adt->cell_adr_table[i].start_sector & 0xffffff ) ==
            0xffffff )
        {
            hb_log( "scan: cell_adr_table[%d].start_sector invalid (0x%x) "
                    "- skipping title", i,
                    d->ifo->vts_c_adt->cell_adr_table[i].start_sector );
            goto fail;
        }
        if( (d->ifo->vts_c_adt->cell_adr_table[i].last_sector & 0xffffff ) ==
            0xffffff )
        {
            hb_log( "scan: cell_adr_table[%d].last_sector invalid (0x%x) "
                    "- skipping title", i,
                    d->ifo->vts_c_adt->cell_adr_table[i].last_sector );
            goto fail;
        }
    }

    if (global_verbosity_level == 4)
    {
        ifo_print( d->reader, d->vts );
    }

    /* Get duration */
    title->duration = d->duration;
    title->hours    =   title->duration / 90000  / 3600;
    title->minutes  = ((title->duration / 90000) % 3600) / 60;
    title->seconds  = ( title->duration / 90000) % 60;

    hb_log( "scan: duration is %02d:%02d:%02d (%"PRId64" ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* ignore titles under 10 seconds because they're often stills or
     * clips with no audio & our preview code doesn't currently handle
     * either of these. */
    if (title->duration < min_duration)
    {
        hb_log( "scan: ignoring title (too short)" );
        goto fail;
    }
    
    if (max_duration > 0 && title->duration > max_duration )
    {
        hb_log( "scan: ignoring title (too long)" );
        goto fail;
    }

    /* Get pgc */
    if (d->pgcn < 1                                 ||
        d->pgcn > d->ifo->vts_pgcit->nr_of_pgci_srp ||
        d->pgcn >= MAX_PGCN)
    {
        hb_log( "invalid PGC ID %d for title %d, skipping", d->pgcn, t );
        goto fail;
    }

    // Check all pgc's for validity
    uint32_t pgcn_map[MAX_PGCN/32];
    pgcn = d->pgcn;
    PgcWalkInit( pgcn_map );
    do
    {
        pgc = d->ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;

        if (!pgc || !pgc->program_map)
        {
            hb_log( "scan: pgc not valid, skipping" );
            goto fail;
        }

        if (pgc->cell_playback == NULL)
        {
            hb_log( "invalid PGC cell_playback table for title %d, skipping", t );
            goto fail;
        }
    } while ((pgcn = NextPgcn(d->ifo, pgcn, pgcn_map)) != 0);

    pgc = d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc;

    hb_log("pgc_id: %d, pgn: %d: pgc: %p", d->pgcn, d->pgn, pgc);
    if (d->pgn > pgc->nr_of_programs)
    {
        hb_log( "invalid PGN %d for title %d, skipping", d->pgn, t );
        goto fail;
    }

    /* Detect languages */
    for (i = 0; i < d->ifo->vtsi_mat->nr_of_vts_audio_streams; i++)
    {
        int audio_format, lang_code, lang_extension, audio_control, position, j;
        hb_audio_t * audio, * audio_tmp;
        iso639_lang_t * lang;

        hb_log( "scan: checking audio %d", i + 1 );

        audio = calloc( sizeof( hb_audio_t ), 1 );

        audio_format   = d->ifo->vtsi_mat->vts_audio_attr[i].audio_format;
        lang_code      = d->ifo->vtsi_mat->vts_audio_attr[i].lang_code;
        lang_extension = d->ifo->vtsi_mat->vts_audio_attr[i].code_extension;
        audio_control  =
            d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc->audio_control[i];

        if (!(audio_control & 0x8000))
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
            free(audio);
            continue;
        }
        const char * name = NULL;
        switch ( lang_extension )
        {
            case 1:
                audio->config.lang.attributes = HB_AUDIO_ATTR_NORMAL;
                break;
            case 2:
                audio->config.lang.attributes = HB_AUDIO_ATTR_VISUALLY_IMPAIRED;
                name = "Visually Impaired";
                break;
            case 3:
                audio->config.lang.attributes = HB_AUDIO_ATTR_COMMENTARY;
                name = "Commentary";
                break;
            case 4:
                audio->config.lang.attributes = HB_AUDIO_ATTR_ALT_COMMENTARY;
                name = "Commentary";
                break;
            default:
                audio->config.lang.attributes = HB_AUDIO_ATTR_NONE;
                break;
        }
        if (name != NULL)
        {
            audio->config.in.name = strdup(name);
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


        snprintf( audio->config.lang.simple,
                  sizeof( audio->config.lang.simple ), "%s",
                  strlen( lang->native_name ) ? lang->native_name : lang->eng_name );
        snprintf( audio->config.lang.iso639_2,
                  sizeof( audio->config.lang.iso639_2 ), "%s", lang->iso639_2 );

        hb_log("scan: id=0x%x, lang=%s (%s), 3cc=%s ext=%i", audio->id,
               audio->config.lang.simple, codec_name,
               audio->config.lang.iso639_2, lang_extension);

        audio->config.index           = hb_list_count(title->list_audio);
        audio->config.in.track        = i;
        audio->config.in.timebase.num = 1;
        audio->config.in.timebase.den = 90000;

        hb_list_add( title->list_audio, audio );
    }

    /* Check for subtitles */
    for( i = 0; i < d->ifo->vtsi_mat->nr_of_vts_subp_streams; i++ )
    {
        int             spu_control, pos, lang_ext = 0;
        iso639_lang_t * lang;

        hb_log( "scan: checking subtitle %d", i + 1 );

        // spu_control
        // 0x80000000 - Subtitle enabled
        // 0x1f000000 - Position mask for 4:3 aspect subtitle track
        // 0x001f0000 - Position mask for Wide Screen subtitle track
        // 0x00001f00 - Position mask for Letterbox subtitle track
        // 0x0000001f - Position mask for Pan&Scan subtitle track
        spu_control =
            d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc->subp_control[i];

        if( !( spu_control & 0x80000000 ) )
        {
            hb_log( "scan: subtitle channel is not active" );
            continue;
        }

        lang_ext = d->ifo->vtsi_mat->vts_subp_attr[i].code_extension;
        lang     = lang_for_code(d->ifo->vtsi_mat->vts_subp_attr[i].lang_code);

        // display_aspect_ratio
        // 0     = 4:3
        // 3     = 16:9
        // other = invalid
        if (d->ifo->vtsi_mat->vts_video_attr.display_aspect_ratio)
        {
            // Add Wide Screen subtitle.
            pos = (spu_control >> 16) & 0x1F;
            add_subtitle(title->list_subtitle, pos, lang, lang_ext,
                (uint8_t*)d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc->palette,
                HB_VOBSUB_STYLE_WIDE);

            // permitted_df
            // 1 - Letterbox not permitted
            // 2 - Pan&Scan not permitted
            // 3 - Letterbox and Pan&Scan not permitted
            if (!(d->ifo->vtsi_mat->vts_video_attr.permitted_df & 1))
            {
                // Letterbox permitted.  Add Letterbox subtitle.
                pos = (spu_control >> 8) & 0x1F;
                add_subtitle(title->list_subtitle, pos, lang, lang_ext,
                    (uint8_t*)d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc->palette,
                    HB_VOBSUB_STYLE_LETTERBOX);
            }
            if (!(d->ifo->vtsi_mat->vts_video_attr.permitted_df & 2))
            {
                // Pan&Scan permitted.  Add Pan&Scan subtitle.
                pos = spu_control & 0x1F;
                add_subtitle(title->list_subtitle, pos, lang, lang_ext,
                    (uint8_t*)d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc->palette,
                    HB_VOBSUB_STYLE_PANSCAN);
            }
        }
        else
        {
            pos = (spu_control >> 24) & 0x1F;
            add_subtitle(title->list_subtitle, pos, lang, lang_ext,
                (uint8_t*)d->ifo->vts_pgcit->pgci_srp[d->pgcn-1].pgc->palette,
                HB_VOBSUB_STYLE_4_3);
        }
    }

    /* Chapters */
    count = hb_list_count(d->list_dvd_chapter);
    hb_log( "scan: title %d has %d chapters", t, count );
    for (i = 0; i < count; i++)
    {
        char chapter_title[80];

        dvd_chapter       = hb_list_item(d->list_dvd_chapter, i);
        chapter           = calloc(sizeof( hb_chapter_t ), 1);
        chapter->index    = i + 1;
        chapter->duration = dvd_chapter->duration;

        snprintf(chapter_title, sizeof(chapter_title), "Chapter %d", chapter->index);
        hb_chapter_set_title(chapter, chapter_title);

        hb_list_add( title->list_chapter, chapter );

        int seconds       = ( chapter->duration + 45000 ) / 90000;
        chapter->hours    = ( seconds / 3600 );
        chapter->minutes  = ( seconds % 3600 ) / 60;
        chapter->seconds  = ( seconds % 60 );

        hb_log( "scan: chap %d, %"PRId64" ms",
                chapter->index, chapter->duration / 90 );
    }

    /* Get aspect. We don't get width/height/rate infos here as
       they tend to be wrong */
    switch (d->ifo->vtsi_mat->vts_video_attr.display_aspect_ratio)
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

    switch (d->ifo->vtsi_mat->vts_video_attr.mpeg_version)
    {
        case 0:
            title->video_codec       = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_MPEG1VIDEO;
            break;
        case 1:
            title->video_codec       = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_MPEG2VIDEO;
            break;
        default:
            hb_log("scan: unknown/reserved MPEG version %d",
                    d->ifo->vtsi_mat->vts_video_attr.mpeg_version);
            title->video_codec       = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_MPEG2VIDEO;
            break;
    }

    hb_log("scan: aspect = %d:%d",
           title->container_dar.num, title->container_dar.den);

    /* This title is ok so far */
    goto cleanup;

fail:
    hb_title_close( &title );

cleanup:
    TitleCloseIfo(d);

    return title;
}

/***********************************************************************
 * hb_dvdnav_title_scan
 **********************************************************************/
static int find_title( hb_list_t * list_title, int title )
{
    int ii;

    for ( ii = 0; ii < hb_list_count( list_title ); ii++ )
    {
        hb_title_t * hbtitle = hb_list_item( list_title, ii );
        if ( hbtitle->index == title )
            return ii;
    }
    return -1;
}

static int skip_to_menu( dvdnav_t * dvdnav, int blocks )
{
    int ii;
    int result, event, len;
    uint8_t buf[HB_DVD_READ_BUFFER_SIZE];

    for ( ii = 0; ii < blocks; ii++ )
    {
        result = dvdnav_get_next_block( dvdnav, buf, &event, &len );
        if ( result == DVDNAV_STATUS_ERR )
        {
            hb_error("dvdnav: Read Error, %s", dvdnav_err_to_string(dvdnav));
            return 0;
        }
        switch ( event )
        {
        case DVDNAV_BLOCK_OK:
            break;

        case DVDNAV_CELL_CHANGE:
        {
        } break;

        case DVDNAV_STILL_FRAME:
        {
            dvdnav_still_event_t *event;
            event = (dvdnav_still_event_t*)buf;
            dvdnav_still_skip( dvdnav );
            if ( event->length == 255 )
            {
                // Infinite still. Can't be the main feature unless
                // you like watching paint dry.
                return 0;
            }
        } break;

        case DVDNAV_WAIT:
            dvdnav_wait_skip( dvdnav );
            break;

        case DVDNAV_STOP:
            return 0;

        case DVDNAV_HOP_CHANNEL:
            break;

        case DVDNAV_NAV_PACKET:
        {
            pci_t *pci = dvdnav_get_current_nav_pci( dvdnav );
            if ( pci == NULL ) break;

            int buttons = pci->hli.hl_gi.btn_ns;

            int title, part;
            result = dvdnav_current_title_info( dvdnav, &title, &part );
            if (result != DVDNAV_STATUS_OK)
            {
                hb_log("dvdnav title info: %s", dvdnav_err_to_string(dvdnav));
            }
            else if ( title == 0 && buttons > 0 )
            {
                // Check button activation duration to see if this
                // isn't another fake menu.
                if ( pci->hli.hl_gi.btn_se_e_ptm - pci->hli.hl_gi.hli_s_ptm >
                     15 * 90000 )
                {
                    // Found what appears to be a good menu.
                    return 1;
                }
            }
        } break;

        case DVDNAV_VTS_CHANGE:
        {
            dvdnav_vts_change_event_t *event;
            event = (dvdnav_vts_change_event_t*)buf;
            // Some discs initialize the vts with the "first play" item
            // and some don't seem to.  So if we see it is uninitialized,
            // set it.
            if ( event->new_vtsN <= 0 )
                dvdnav_title_play( dvdnav, 1 );
        } break;

        case DVDNAV_HIGHLIGHT:
            break;

        case DVDNAV_AUDIO_STREAM_CHANGE:
            break;

        case DVDNAV_SPU_STREAM_CHANGE:
            break;

        case DVDNAV_SPU_CLUT_CHANGE:
            break;

        case DVDNAV_NOP:
            break;

        default:
            break;
        }
    }
    return 0;
}

static int try_button( dvdnav_t * dvdnav, int button, hb_list_t * list_title )
{
    int result, event, len;
    uint8_t buf[HB_DVD_READ_BUFFER_SIZE];
    int ii, jj;
    int32_t cur_title = 0, title, part;
    uint64_t longest_duration = 0;
    int longest = -1;

    pci_t *pci = dvdnav_get_current_nav_pci( dvdnav );

    result = dvdnav_button_select_and_activate( dvdnav, pci, button + 1 );
    if (result != DVDNAV_STATUS_OK)
    {
        hb_log("dvdnav_button_select_and_activate: %s", dvdnav_err_to_string(dvdnav));
    }

    result = dvdnav_current_title_info( dvdnav, &title, &part );
    if (result != DVDNAV_STATUS_OK)
        hb_log("dvdnav cur title info: %s", dvdnav_err_to_string(dvdnav));

    cur_title = title;

    for (jj = 0; jj < 10; jj++)
    {
        for (ii = 0; ii < 2000; ii++)
        {
            result = dvdnav_get_next_block( dvdnav, buf, &event, &len );
            if ( result == DVDNAV_STATUS_ERR )
            {
                hb_error("dvdnav: Read Error, %s", dvdnav_err_to_string(dvdnav));
                goto done;
            }
            switch ( event )
            {
            case DVDNAV_BLOCK_OK:
                break;

            case DVDNAV_CELL_CHANGE:
            {
                result = dvdnav_current_title_info( dvdnav, &title, &part );
                if (result != DVDNAV_STATUS_OK)
                    hb_log("dvdnav title info: %s", dvdnav_err_to_string(dvdnav));

                cur_title = title;
                // Note, some "fake" titles have long advertised durations
                // but then jump to the real title early in playback.
                // So keep reading after finding a long title to detect
                // such cases.
            } break;

            case DVDNAV_STILL_FRAME:
            {
                dvdnav_still_event_t *event;
                event = (dvdnav_still_event_t*)buf;
                dvdnav_still_skip( dvdnav );
                if ( event->length == 255 )
                {
                    // Infinite still. Can't be the main feature unless
                    // you like watching paint dry.
                    goto done;
                }
            } break;

            case DVDNAV_WAIT:
                dvdnav_wait_skip( dvdnav );
                break;

            case DVDNAV_STOP:
                goto done;

            case DVDNAV_HOP_CHANNEL:
                break;

            case DVDNAV_NAV_PACKET:
            {
            } break;

            case DVDNAV_VTS_CHANGE:
            {
                result = dvdnav_current_title_info( dvdnav, &title, &part );
                if (result != DVDNAV_STATUS_OK)
                    hb_log("dvdnav title info: %s", dvdnav_err_to_string(dvdnav));

                cur_title = title;
                // Note, some "fake" titles have long advertised durations
                // but then jump to the real title early in playback.
                // So keep reading after finding a long title to detect
                // such cases.
            } break;

            case DVDNAV_HIGHLIGHT:
                break;

            case DVDNAV_AUDIO_STREAM_CHANGE:
                break;

            case DVDNAV_SPU_STREAM_CHANGE:
                break;

            case DVDNAV_SPU_CLUT_CHANGE:
                break;

            case DVDNAV_NOP:
                break;

            default:
                break;
            }
        }
        // Check if the current title is long enough to qualify
        // as the main feature.
        if ( cur_title > 0 )
        {
            hb_title_t * hbtitle;
            int index;
            index = find_title( list_title, cur_title );
            hbtitle = hb_list_item( list_title, index );
            if ( hbtitle != NULL )
            {
                if ( hbtitle->duration / 90000 > 10 * 60 )
                {
                    hb_deep_log( 3, "dvdnav: Found candidate feature title %d duration %02d:%02d:%02d on button %d",
                    cur_title, hbtitle->hours, hbtitle->minutes,
                    hbtitle->seconds, button+1 );
                    return cur_title;
                }
                if ( hbtitle->duration > longest_duration )
                {
                    longest_duration = hbtitle->duration;
                    longest = title;
                }
            }
            // Some titles have long lead-ins. Try skipping it.
            dvdnav_next_pg_search( dvdnav );
        }
    }

done:
    if ( longest != -1 )
    {
        hb_title_t * hbtitle;
        int index;
        index = find_title( list_title, longest );
        hbtitle = hb_list_item( list_title, index );
        if ( hbtitle != NULL )
        {
            hb_deep_log( 3, "dvdnav: Found candidate feature title %d duration %02d:%02d:%02d on button %d",
            longest, hbtitle->hours, hbtitle->minutes,
            hbtitle->seconds, button+1 );
        }
    }
    return longest;
}

static int try_menu(
    hb_dvdnav_t * d,
    hb_list_t * list_title,
    DVDMenuID_t menu,
    uint64_t fallback_duration )
{
    int result, event, len;
    uint8_t buf[HB_DVD_READ_BUFFER_SIZE];
    int ii, jj;
    int32_t cur_title, title, part;
    uint64_t longest_duration = 0;
    int longest = -1;

    // A bit of a hack here.  Abusing Escape menu to mean use whatever
    // current menu is already set.
    if ( menu != DVD_MENU_Escape )
    {
        result = dvdnav_menu_call( d->dvdnav, menu );
        if ( result != DVDNAV_STATUS_OK )
        {
            // Sometimes the "first play" item doesn't initialize the
            // initial VTS. So do it here.
            dvdnav_title_play( d->dvdnav, 1 );
            result = dvdnav_menu_call( d->dvdnav, menu );
            if ( result != DVDNAV_STATUS_OK )
            {
                hb_error("dvdnav: Can not set dvd menu, %s", dvdnav_err_to_string(d->dvdnav));
                goto done;
            }
        }
    }

    result = dvdnav_current_title_info( d->dvdnav, &title, &part );
    if (result != DVDNAV_STATUS_OK)
        hb_log("dvdnav title info: %s", dvdnav_err_to_string(d->dvdnav));

    cur_title = title;

    for (jj = 0; jj < 4; jj++)
    {
        for (ii = 0; ii < 4000; ii++)
        {
            result = dvdnav_get_next_block( d->dvdnav, buf, &event, &len );
            if ( result == DVDNAV_STATUS_ERR )
            {
                hb_error("dvdnav: Read Error, %s", dvdnav_err_to_string(d->dvdnav));
                goto done;
            }
            switch ( event )
            {
            case DVDNAV_BLOCK_OK:
                break;

            case DVDNAV_CELL_CHANGE:
            {
                result = dvdnav_current_title_info( d->dvdnav, &title, &part );
                if (result != DVDNAV_STATUS_OK)
                    hb_log("dvdnav title info: %s", dvdnav_err_to_string(d->dvdnav));
                cur_title = title;
            } break;

            case DVDNAV_STILL_FRAME:
            {
                dvdnav_still_event_t *event;
                event = (dvdnav_still_event_t*)buf;
                dvdnav_still_skip( d->dvdnav );
                if ( event->length == 255 )
                {
                    // Infinite still.  There won't be any menus after this.
                    goto done;
                }
            } break;

            case DVDNAV_WAIT:
                dvdnav_wait_skip( d->dvdnav );
                break;

            case DVDNAV_STOP:
                goto done;

            case DVDNAV_HOP_CHANNEL:
                break;

            case DVDNAV_NAV_PACKET:
            {
                pci_t *pci = dvdnav_get_current_nav_pci( d->dvdnav );
                int kk;
                int buttons;
                if ( pci == NULL ) break;

                buttons = pci->hli.hl_gi.btn_ns;

                // If we are on a menu that has buttons and
                // the button activation duration is long enough
                // that this isn't another fake menu.
                if ( cur_title == 0 && buttons > 0 &&
                     pci->hli.hl_gi.btn_se_e_ptm - pci->hli.hl_gi.hli_s_ptm >
                     15 * 90000 )
                {
                    for (kk = 0; kk < buttons; kk++)
                    {
                        dvdnav_t *dvdnav_copy;

                        result = dvdnav_dup( &dvdnav_copy, d->dvdnav );
                        if (result != DVDNAV_STATUS_OK)
                        {
                            hb_log("dvdnav dup failed: %s", dvdnav_err_to_string(d->dvdnav));
                            goto done;
                        }
                        title = try_button( dvdnav_copy, kk, list_title );
                        dvdnav_free_dup( dvdnav_copy );

                        if ( title >= 0 )
                        {
                            hb_title_t * hbtitle;
                            int index;
                            index = find_title( list_title, title );
                            hbtitle = hb_list_item( list_title, index );
                            if ( hbtitle != NULL )
                            {
                                if ( hbtitle->duration > longest_duration )
                                {
                                    longest_duration = hbtitle->duration;
                                    longest = title;
                                    if ((float)fallback_duration * 0.75 < longest_duration)
                                        goto done;
                                }
                            }
                        }
                    }
                    goto done;
                }
            } break;

            case DVDNAV_VTS_CHANGE:
            {
                result = dvdnav_current_title_info( d->dvdnav, &title, &part );
                if (result != DVDNAV_STATUS_OK)
                    hb_log("dvdnav title info: %s", dvdnav_err_to_string(d->dvdnav));
                cur_title = title;
            } break;

            case DVDNAV_HIGHLIGHT:
                break;

            case DVDNAV_AUDIO_STREAM_CHANGE:
                break;

            case DVDNAV_SPU_STREAM_CHANGE:
                break;

            case DVDNAV_SPU_CLUT_CHANGE:
                break;

            case DVDNAV_NOP:
                break;

            default:
                break;
            }
        }
        // Sometimes the menu is preceded by a intro that just
        // gets restarted when hitting the menu button. So
        // try skipping with the skip forward button.  Then
        // try hitting the menu again.
        if ( !(jj & 1) )
        {
            dvdnav_next_pg_search( d->dvdnav );
        }
        else
        {
            dvdnav_menu_call( d->dvdnav, menu );
        }
    }

done:
    return longest;
}

static int hb_dvdnav_main_feature( hb_dvd_t * e, hb_list_t * list_title )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    int longest_root = -1;
    int longest_title = -1;
    int longest_fallback = 0;
    int ii;
    uint64_t longest_duration_root = 0;
    uint64_t longest_duration_title = 0;
    uint64_t longest_duration_fallback = 0;
    uint64_t avg_duration = 0;
    int avg_cnt = 0;
    hb_title_t * title;
    int index;

    hb_deep_log( 2, "dvdnav: Searching menus for main feature" );
    for ( ii = 0; ii < hb_list_count( list_title ); ii++ )
    {
        title = hb_list_item( list_title, ii );
        if ( title->duration > longest_duration_fallback )
        {
            longest_duration_fallback = title->duration;
            longest_fallback = title->index;
        }
        if ( title->duration > 90000LL * 60 * 30 )
        {
            avg_duration += title->duration;
            avg_cnt++;
        }
    }
    if ( avg_cnt )
        avg_duration /= avg_cnt;

    index = find_title( list_title, longest_fallback );
    title = hb_list_item( list_title, index );
    if ( title )
    {
        hb_deep_log( 2, "dvdnav: Longest title %d duration %02d:%02d:%02d",
                    longest_fallback, title->hours, title->minutes,
                    title->seconds );
    }

    dvdnav_reset( d->dvdnav );
    if ( skip_to_menu( d->dvdnav, 2000 ) )
    {
        longest_root = try_menu( d, list_title, DVD_MENU_Escape, longest_duration_fallback );
        if ( longest_root >= 0 )
        {
            index = find_title( list_title, longest_root );
            title = hb_list_item( list_title, index );
            if ( title )
            {
                longest_duration_root = title->duration;
                hb_deep_log( 2, "dvdnav: Found first-play title %d duration %02d:%02d:%02d",
                            longest_root, title->hours, title->minutes, title->seconds );
            }
        }
        else
        {
            hb_deep_log( 2, "dvdnav: No first-play menu title found" );
        }
    }

    if ( longest_root < 0 ||
         (float)longest_duration_fallback * 0.7 > longest_duration_root)
    {
        longest_root = try_menu( d, list_title, DVD_MENU_Root, longest_duration_fallback );
        if ( longest_root >= 0 )
        {
            index = find_title( list_title, longest_root );
            title = hb_list_item( list_title, index );
            if ( title )
            {
                longest_duration_root = title->duration;
                hb_deep_log( 2, "dvdnav: Found root title %d duration %02d:%02d:%02d",
                            longest_root, title->hours, title->minutes, title->seconds );
            }
        }
        else
        {
            hb_deep_log( 2, "dvdnav: No root menu title found" );
        }
    }

    if ( longest_root < 0 ||
         (float)longest_duration_fallback * 0.7 > longest_duration_root)
    {
        longest_title = try_menu( d, list_title, DVD_MENU_Title, longest_duration_fallback );
        if ( longest_title >= 0 )
        {
            index = find_title( list_title, longest_title );
            title = hb_list_item( list_title, index );
            if ( title )
            {
                longest_duration_title = title->duration;
                hb_deep_log( 2, "dvdnav: found title %d duration %02d:%02d:%02d",
                            longest_title, title->hours, title->minutes,
                            title->seconds );
            }
        }
        else
        {
            hb_deep_log( 2, "dvdnav: No title menu title found" );
        }
    }

    uint64_t longest_duration;
    int longest;

    if ( longest_duration_root > longest_duration_title )
    {
        longest_duration = longest_duration_root;
        longest = longest_root;
    }
    else
    {
        longest_duration = longest_duration_title;
        longest = longest_title;
    }
    if ((float)longest_duration_fallback * 0.7 > longest_duration &&
        longest_duration < 90000LL * 60 * 30 )
    {
        float factor = (float)avg_duration / longest_duration;
        if ( factor > 1 )
            factor = 1 / factor;
        if ( avg_cnt > 10 && factor < 0.7 )
        {
            longest = longest_fallback;
            hb_deep_log( 2, "dvdnav: Using longest title %d", longest );
        }
    }
    return longest;
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
    hb_dvd_chapter_t *chapter;
    dvdnav_status_t result;

    if ( d->stopped && !hb_dvdnav_reset(d) )
    {
        return 0;
    }
    if (!TitleOpenIfo(d, t))
    {
        return 0;
    }
    dvdnav_reset( d->dvdnav );
    chapter = hb_list_item( d->list_dvd_chapter, c - 1);
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
    d->stopped = 0;
    d->chapter = 0;
    d->cell = 0;
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
    uint64_t pgc_offset = 0;
    uint64_t chap_offset = 0;
    hb_dvd_chapter_t *pgc_change = hb_list_item(d->list_dvd_chapter, 0 );
    for ( ii = 0; ii < hb_list_count( d->list_dvd_chapter ); ++ii )
    {
        hb_dvd_chapter_t *chapter = hb_list_item( d->list_dvd_chapter, ii );
        uint64_t chap_len = chapter->block_end - chapter->block_start + 1;

        if ( chapter->pgcn != pgc_change->pgcn )
        {
            // this chapter's in a different pgc from the previous - note the
            // change so we can make sector offset's be pgc relative.
            pgc_offset = chap_offset;
            pgc_change = chapter;
        }
        if ( chap_offset <= sector && sector < chap_offset + chap_len )
        {
            // this chapter contains the sector we want - see if it's in a
            // different pgc than the one we're currently in.
            int32_t title, pgcn, pgn;
            if (dvdnav_current_title_program( d->dvdnav, &title, &pgcn, &pgn ) != DVDNAV_STATUS_OK)
                hb_log("dvdnav cur pgcn err: %s", dvdnav_err_to_string(d->dvdnav));
            // If we find ourselves in a new title, it means a title
            // transition was made while reading data.  Jumping between
            // titles can cause the vm to get into a bad state.  So
            // reset the vm in this case.
            if ( d->title != title )
                dvdnav_reset( d->dvdnav );

            if ( d->title != title || chapter->pgcn != pgcn )
            {
                // this chapter is in a different pgc - switch to it.
                if (dvdnav_program_play(d->dvdnav, d->title, chapter->pgcn, chapter->pgn) != DVDNAV_STATUS_OK)
                    hb_log("dvdnav prog play err: %s", dvdnav_err_to_string(d->dvdnav));
            }
            // seek sectors are pgc-relative so remove the pgc start sector.
            sector -= pgc_offset;
            break;
        }
        chap_offset += chap_len;
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
            hb_error("dvdnav: Read Error, %s", dvdnav_err_to_string(d->dvdnav));
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
    d->chapter = 0;
    d->cell = 0;
    return 1;
}

/***********************************************************************
 * hb_dvdnav_read
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * hb_dvdnav_read( hb_dvd_t * e )
{
    hb_dvdnav_t * d = &(e->dvdnav);
    int result, event, len;
    int chapter = 0;
    int error_count = 0;
    hb_buffer_t *b = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );

    while ( 1 )
    {
        if (d->stopped)
        {
            hb_buffer_close( &b );
            return NULL;
        }
        result = dvdnav_get_next_block( d->dvdnav, b->data, &event, &len );
        if ( result == DVDNAV_STATUS_ERR )
        {
            hb_error("dvdnav: Read Error, %s", dvdnav_err_to_string(d->dvdnav));
            if (dvdnav_sector_search(d->dvdnav, 1, SEEK_CUR) != DVDNAV_STATUS_OK)
            {
                hb_error( "dvd: dvdnav_sector_search failed - %s",
                        dvdnav_err_to_string(d->dvdnav) );
                hb_buffer_close( &b );
                hb_set_work_error(d->h, HB_ERROR_READ);
                return NULL;
            }
            error_count++;
            if (error_count > 500)
            {
                hb_error("dvdnav: Error, too many consecutive read errors");
                hb_buffer_close( &b );
                hb_set_work_error(d->h, HB_ERROR_READ);
                return NULL;
            }
            continue;
        }
        switch ( event )
        {
        case DVDNAV_BLOCK_OK:
            // We have received a regular block of the currently playing
            // MPEG stream.
            b->s.new_chap = chapter;
            return b;

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
            {
                int tt = 0, pgcn = 0, pgn = 0;

                dvdnav_current_title_program(d->dvdnav, &tt, &pgcn, &pgn);
                if (tt != d->title)
                {
                    // Transition to another title signals that we are done.
                    hb_buffer_close( &b );
                    hb_deep_log(2, "dvdnav: vts change, found next title");
                    if (error_count > 0)
                    {
                        // Last read attempt failed
                        hb_set_work_error(d->h, HB_ERROR_READ);
                    }
                    return NULL;
                }
            }
            break;

        case DVDNAV_CELL_CHANGE:
            /*
            * Some status information like the current Title and Part
            * numbers do not change inside a cell. Therefore this event
            * can be used to query such information only when necessary
            * and update the decoding/displaying accordingly.
            */
            {
                dvdnav_cell_change_event_t * cell_event;
                int tt = 0, pgcn = 0, pgn = 0, c;

                cell_event = (dvdnav_cell_change_event_t*)b->data;

                dvdnav_current_title_program(d->dvdnav, &tt, &pgcn, &pgn);
                if (tt != d->title)
                {
                    // Transition to another title signals that we are done.
                    hb_buffer_close( &b );
                    hb_deep_log(2, "dvdnav: cell change, found next title");
                    if (error_count > 0)
                    {
                        // Last read attempt failed
                        hb_set_work_error(d->h, HB_ERROR_READ);
                    }
                    return NULL;
                }
                c = FindChapterIndex(d->list_dvd_chapter, pgcn, pgn);
                if (c != d->chapter)
                {
                    if (c < d->chapter)
                    {
                        // Some titles end with a 'link' back to the beginning so
                        // a transition to an earlier chapter means we're done.
                        hb_buffer_close( &b );
                        hb_deep_log(2, "dvdnav: cell change, previous chapter");
                        if (error_count > 0)
                        {
                            // Last read attempt failed
                            hb_set_work_error(d->h, HB_ERROR_READ);
                        }
                        return NULL;
                    }
                    chapter = d->chapter = c;
                }
                else if ( cell_event->cellN <= d->cell )
                {
                    hb_buffer_close( &b );
                    hb_deep_log(2, "dvdnav: cell change, previous cell");
                    if (error_count > 0)
                    {
                        // Last read attempt failed
                        hb_set_work_error(d->h, HB_ERROR_READ);
                    }
                    return NULL;
                }
                d->cell = cell_event->cellN;
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
            b->s.new_chap = chapter;
            return b;

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
            hb_buffer_close( &b );
            hb_deep_log(2, "dvdnav: stop");
            if (error_count > 0)
            {
                // Last read attempt failed
                hb_set_work_error(d->h, HB_ERROR_READ);
            }
            return NULL;

        default:
            break;
        }
    }
    hb_buffer_close( &b );
    return NULL;
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
    c = FindChapterIndex( d->list_dvd_chapter, pgcn, pgn );
    return c;
}

/***********************************************************************
 * hb_dvdnav_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
static void hb_dvdnav_close( hb_dvd_t ** _d )
{
    hb_dvdnav_t      * d = &((*_d)->dvdnav);

    if (d->dvdnav) dvdnav_close( d->dvdnav );
    if (d->vmg)    ifoClose( d->vmg );
    TitleCloseIfo(d);
    if (d->reader) DVDClose( d->reader );

    free(d->path);


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
    hb_dvd_chapter_t * chapter;

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
    memset(pgcn_map, 0, sizeof(uint32_t) * MAX_PGCN/32);
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

static int TitleOpenIfo(hb_dvdnav_t * d, int t)
{
    int                pgcn, pgn, pgcn_end, i, c;
    pgc_t            * pgc;
    int                cell_cur;
    hb_dvd_chapter_t * dvd_chapter;
    uint64_t           duration;

    if (d->title == t && d->ifo != NULL)
    {
        // Already opened
        return 0;
    }

    // Close previous if open
    TitleCloseIfo(d);

    /* VTS which our title is in */
    d->vts = d->vmg->tt_srpt->title[t-1].title_set_nr;

    if (!d->vts)
    {
        /* A VTS of 0 means the title wasn't found in the title set */
        hb_log("Invalid VTS (title set) number: %i", d->vts);
        goto fail;
    }

    if(!(d->ifo = ifoOpen(d->reader, d->vts)))
    {
        hb_log( "ifoOpen failed" );
        goto fail;
    }

    int title_ttn = d->vmg->tt_srpt->title[t-1].vts_ttn;
    if ( title_ttn < 1 || title_ttn > d->ifo->vts_ptt_srpt->nr_of_srpts )
    {
        hb_log( "invalid VTS PTT offset %d for title %d, skipping", title_ttn, t );
        goto fail;
    }

    d->duration = 0LL;
    d->pgcn     = -1;
    d->pgn      = 1;
    for (i = 0; i < d->ifo->vts_ptt_srpt->title[title_ttn-1].nr_of_ptts; i++)
    {
        int blocks = 0;

        duration = PttDuration(d->ifo, title_ttn, i+1, &blocks, &pgcn_end);
        pgcn     = d->ifo->vts_ptt_srpt->title[title_ttn-1].ptt[i].pgcn;
        pgn      = d->ifo->vts_ptt_srpt->title[title_ttn-1].ptt[i].pgn;
        if (duration > d->duration)
        {
            d->pgcn              = pgcn;
            d->pgn               = pgn;
            d->duration          = duration;
            d->title_block_count = blocks;
        }
        else if (pgcn == d->pgcn && pgn < d->pgn)
        {
            d->pgn               = pgn;
            d->title_block_count = blocks;
        }
    }

    /* Check pgc */
    if ( d->pgcn < 1 || d->pgcn > d->ifo->vts_pgcit->nr_of_pgci_srp || d->pgcn >= MAX_PGCN)
    {
        hb_log( "invalid PGC ID %d for title %d, skipping", d->pgcn, t );
        goto fail;
    }

    /* Chapters */
    d->list_dvd_chapter = hb_list_init();

    uint32_t pgcn_map[MAX_PGCN/32];
    PgcWalkInit( pgcn_map );
    pgcn = d->pgcn;
    pgn  = d->pgn;
    c = 0;
    do
    {
        pgc = d->ifo->vts_pgcit->pgci_srp[pgcn-1].pgc;

        for (i = pgn; i <= pgc->nr_of_programs; i++)
        {
            int cell_start, cell_end;

            dvd_chapter = calloc(sizeof(hb_dvd_chapter_t), 1);

            dvd_chapter->pgcn  = pgcn;
            dvd_chapter->pgn   = i;
            dvd_chapter->index = c + 1;

            cell_start  = pgc->program_map[i-1] - 1;
            dvd_chapter->block_start = pgc->cell_playback[cell_start].first_sector;

            // if there are no more programs in this pgc, the end cell is the
            // last cell. Otherwise it's the cell before the start cell of the
            // next program.
            if (i == pgc->nr_of_programs)
            {
                cell_end = pgc->nr_of_cells - 1;
            }
            else
            {
                cell_end = pgc->program_map[i] - 2;
            }
            dvd_chapter->block_end = pgc->cell_playback[cell_end].last_sector;

            /* duration */
            dvd_chapter->duration = 0;

            cell_cur = cell_start;
            while( cell_cur <= cell_end )
            {
#define cp pgc->cell_playback[cell_cur]
                dvd_chapter->duration += 90LL * dvdtime2msec(&cp.playback_time);
#undef cp
                cell_cur = FindNextCell( pgc, cell_cur );
            }
            hb_list_add(d->list_dvd_chapter, dvd_chapter);
            c++;
        }
        pgn = 1;
    } while ((pgcn = NextPgcn(d->ifo, pgcn, pgcn_map)) != 0);

    d->title = t;
    return 1;

fail:
    TitleCloseIfo(d);
    return 0;
}

static void TitleCloseIfo(hb_dvdnav_t * d)
{
    hb_dvd_chapter_t * chapter;
    while ((chapter = hb_list_item(d->list_dvd_chapter, 0)))
    {
        hb_list_rem(d->list_dvd_chapter, chapter );
        free(chapter);
    }
    hb_list_close(&d->list_dvd_chapter);

    if (d->ifo)
    {
        ifoClose(d->ifo);
    }
    d->ifo   = NULL;
    d->title = 0;
    d->vts   = 0;
    d->pgcn  = 0;
    d->pgn   = 0;
}
