/* $Id: dvd.c,v 1.12 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "lang.h"

#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"

struct hb_dvd_s
{
    char         * path;

    dvd_reader_t * reader;
    ifo_handle_t * vmg;

    int            vts;
    int            ttn;
    ifo_handle_t * ifo;
    dvd_file_t   * file;

    pgc_t        * pgc;
    int            cell_start;
    int            cell_end;
    int            title_start;
    int            title_end;
    int            title_block_count;
    int            cell_cur;
    int            cell_next;
    int            cell_overlap;
    int            block;
    int            pack_len;
    int            next_vobu;
    int            in_cell;
    int            in_sync;
    uint16_t       cur_vob_id;
    uint8_t        cur_cell_id;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void FindNextCell( hb_dvd_t * );
static int  dvdtime2msec( dvd_time_t * );

char * hb_dvd_name( char * path )
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
 * hb_dvd_init
 ***********************************************************************
 *
 **********************************************************************/
hb_dvd_t * hb_dvd_init( char * path )
{
    hb_dvd_t * d;

    d = calloc( sizeof( hb_dvd_t ), 1 );

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

    return d;

fail:
    if( d->vmg )    ifoClose( d->vmg );
    if( d->reader ) DVDClose( d->reader );
    free( d );
    return NULL;
}

/***********************************************************************
 * hb_dvd_title_count
 **********************************************************************/
int hb_dvd_title_count( hb_dvd_t * d )
{
    return d->vmg->tt_srpt->nr_of_srpts;
}

/***********************************************************************
 * hb_dvd_title_scan
 **********************************************************************/
hb_title_t * hb_dvd_title_scan( hb_dvd_t * d, int t )
{

    hb_title_t   * title;
    ifo_handle_t * vts = NULL;
    int            pgc_id, pgn, i;
    hb_chapter_t * chapter;
    int            c;
    uint64_t       duration;
    float          duration_correction;
    unsigned char  unused[1024];

    hb_log( "scan: scanning title %d", t );

    title = hb_title_init( d->path, t );
    if( DVDUDFVolumeInfo( d->reader, title->name, sizeof( title->name ),
                          unused, sizeof( unused ) ) )
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
    if( !( vts = ifoOpen( d->reader, title->vts ) ) )
    {
        hb_error( "scan: ifoOpen failed" );
        goto fail;
    }

    if( global_verbosity_level == 3 )
    {
        ifoPrint( d->reader, title->vts );
    }

    /* Position of the title in the VTS */
    title->ttn = d->vmg->tt_srpt->title[t-1].vts_ttn;

    /* Get pgc */
    pgc_id = vts->vts_ptt_srpt->title[title->ttn-1].ptt[0].pgcn;
    pgn    = vts->vts_ptt_srpt->title[title->ttn-1].ptt[0].pgn;
    d->pgc = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;

    hb_log("pgc_id: %d, pgn: %d: pgc: 0x%x", pgc_id, pgn, d->pgc);

    if( !d->pgc )
    {
        hb_error( "scan: pgc not valid, skipping" );
        goto fail;
    }

    if( pgn <= 0 || pgn > 99 )
    {
        hb_error( "scan: pgn %d not valid, skipping", pgn );
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

    hb_log( "scan: vts=%d, ttn=%d, cells=%d->%d, blocks=%d->%d, "
            "%d blocks", title->vts, title->ttn, title->cell_start,
            title->cell_end, title->block_start, title->block_end,
            title->block_count );

    /* Get duration */
    title->duration = 90LL * dvdtime2msec( &d->pgc->playback_time );
    title->hours    = title->duration / 90000 / 3600;
    title->minutes  = ( ( title->duration / 90000 ) % 3600 ) / 60;
    title->seconds  = ( title->duration / 90000 ) % 60;
    hb_log( "scan: duration is %02d:%02d:%02d (%lld ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* ignore titles under 10 seconds because they're often stills or
     * clips with no audio & our preview code doesn't currently handle
     * either of these. */
    if( title->duration < 900000LL )
    {
        hb_log( "scan: ignoring title (too short)" );
        goto fail;
    }

    /* Detect languages */
    for( i = 0; i < vts->vtsi_mat->nr_of_vts_audio_streams; i++ )
    {
        hb_audio_t * audio, * audio_tmp;
        int          audio_format, lang_code, audio_control,
                     position, j;
        iso639_lang_t * lang;
        int lang_extension = 0;

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

        lang = lang_for_code( vts->vtsi_mat->vts_audio_attr[i].lang_code );

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
            vts->vts_pgcit->pgci_srp[pgc_id-1].pgc->palette,
            16 * sizeof( uint32_t ) );

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
    hb_log( "scan: title %d has %d chapters", t,
            vts->vts_ptt_srpt->title[title->ttn-1].nr_of_ptts );
    for( i = 0, c = 1;
         i < vts->vts_ptt_srpt->title[title->ttn-1].nr_of_ptts; i++ )
    {
        int pgc_id_next, pgn_next;
        pgc_t * pgc_next;

        chapter = calloc( sizeof( hb_chapter_t ), 1 );
        /* remember the on-disc chapter number */
        chapter->index = i + 1;

        pgc_id = vts->vts_ptt_srpt->title[title->ttn-1].ptt[i].pgcn;
        pgn    = vts->vts_ptt_srpt->title[title->ttn-1].ptt[i].pgn;
        d->pgc = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;

        /* Start cell */
        chapter->cell_start  = d->pgc->program_map[pgn-1] - 1;
        chapter->block_start =
            d->pgc->cell_playback[chapter->cell_start].first_sector;

        /* End cell */
        if( i != vts->vts_ptt_srpt->title[title->ttn-1].nr_of_ptts - 1 )
        {
            /* The cell before the starting cell of the next chapter,
               or... */
            pgc_id_next = vts->vts_ptt_srpt->title[title->ttn-1].ptt[i+1].pgcn;
            pgn_next    = vts->vts_ptt_srpt->title[title->ttn-1].ptt[i+1].pgn;
            pgc_next    = vts->vts_pgcit->pgci_srp[pgc_id_next-1].pgc;
            chapter->cell_end = pgc_next->program_map[pgn_next-1] - 2;
            if( chapter->cell_end < 0 )
            {
                /* Huh? */
                free( chapter );
                continue;
            }
        }
        else
        {
            /* ... the last cell of the title */
            chapter->cell_end = title->cell_end;
        }
        chapter->block_end =
            d->pgc->cell_playback[chapter->cell_end].last_sector;

        /* Block count, duration */
        pgc_id = vts->vts_ptt_srpt->title[title->ttn-1].ptt[0].pgcn;
        pgn    = vts->vts_ptt_srpt->title[title->ttn-1].ptt[0].pgn;
        d->pgc = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;
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
        c++;
    }

    /* The durations we get for chapters aren't precise. Scale them so
       the total matches the title duration */
    duration = 0;
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        chapter = hb_list_item( title->list_chapter, i );
        duration += chapter->duration;
    }
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
    switch( vts->vtsi_mat->vts_video_attr.display_aspect_ratio )
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
    if( vts ) ifoClose( vts );

    return title;
}

/***********************************************************************
 * hb_dvd_start
 ***********************************************************************
 * Title and chapter start at 1
 **********************************************************************/
int hb_dvd_start( hb_dvd_t * d, int title, int chapter )
{
    int pgc_id, pgn;
    int i;

    /* Open the IFO and the VOBs for this title */
    d->vts = d->vmg->tt_srpt->title[title-1].title_set_nr;
    d->ttn = d->vmg->tt_srpt->title[title-1].vts_ttn;
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
 * hb_dvd_stop
 ***********************************************************************
 *
 **********************************************************************/
void hb_dvd_stop( hb_dvd_t * d )
{
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
 * hb_dvd_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_dvd_seek( hb_dvd_t * d, float f )
{
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

            /* Now let hb_dvd_read find the next VOBU */
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
 * hb_dvd_read
 ***********************************************************************
 *
 **********************************************************************/
int hb_dvd_read( hb_dvd_t * d, hb_buffer_t * b )
{
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
            return 0;

        for( ;; )
        {
            int block, pack_len, next_vobu, read_retry;

            for( read_retry = 0; read_retry < 3; read_retry++ )
            {
                if( DVDReadBlocks( d->file, d->next_vobu, 1, b->data ) == 1 )
                {
                    /*
                     * Successful read.
                     */
                    break;
                } else {
                    hb_log( "dvd: Read Error on blk %d, attempt %d",
                            d->next_vobu, read_retry );
                }
            }

            if( read_retry == 3 )
            {
                hb_log( "dvd: vobu read error blk %d - skipping to cell %d",
                        d->next_vobu, d->cell_next );
                d->cell_cur  = d->cell_next;
                if ( d->cell_cur > d->cell_end )
                    return 0;
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
                hb_log( "dvd: couldn't find a VOBU after 1024 blocks" );
                return 0;
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
                    hb_error( "dvd: assuming missed end of cell %d", d->cell_cur );
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
                    b->new_chap = hb_dvd_is_break( d );
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

    return 1;
}

/***********************************************************************
 * hb_dvd_chapter
 ***********************************************************************
 * Returns in which chapter the next block to be read is.
 * Chapter numbers start at 1.
 **********************************************************************/
int hb_dvd_chapter( hb_dvd_t * d )
{
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
 * hb_dvd_is_break
 ***********************************************************************
 * Returns chapter number if the current block is a new chapter start
 **********************************************************************/
int hb_dvd_is_break( hb_dvd_t * d )
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
 * hb_dvd_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_dvd_close( hb_dvd_t ** _d )
{
    hb_dvd_t * d = *_d;

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
 * FindNextCell
 ***********************************************************************
 * Assumes pgc and cell_cur are correctly set, and sets cell_next to the
 * cell to be read when we will be done with cell_cur.
 **********************************************************************/
static void FindNextCell( hb_dvd_t * d )
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
        ms += ((dt->frame_u & 0x30) >> 3) * 5 +
              (dt->frame_u & 0x0f) * 1000.0 / fps;
    }

    return ms;
}
