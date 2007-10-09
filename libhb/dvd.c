/* $Id: dvd.c,v 1.12 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
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
        hb_error( "dvd: DVDOpen failed (%s)", path );
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
    hb_chapter_t * chapter, * chapter_old;
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

    hb_log( "scan: opening IFO for VTS %d", title->vts );
    if( !( vts = ifoOpen( d->reader, title->vts ) ) )
    {
        hb_error( "scan: ifoOpen failed" );
        goto fail;
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

    if( title->block_count < 2048  )
    {
        hb_log( "scan: title too short (%d blocks), ignoring",
                title->block_count );
        goto fail;
    }


    /* Get duration */
    title->duration = 90LL * dvdtime2msec( &d->pgc->playback_time );
    title->hours    = title->duration / 90000 / 3600;
    title->minutes  = ( ( title->duration / 90000 ) % 3600 ) / 60;
    title->seconds  = ( title->duration / 90000 ) % 60;
    hb_log( "scan: duration is %02d:%02d:%02d (%lld ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* Discard titles under 10 seconds */
    if( !( title->hours | title->minutes ) && title->seconds < 10 )
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

        hb_log( "scan: checking audio %d", i + 1 );

        audio = calloc( sizeof( hb_audio_t ), 1 );

        audio_format  = vts->vtsi_mat->vts_audio_attr[i].audio_format;
        lang_code     = vts->vtsi_mat->vts_audio_attr[i].lang_code;
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
                audio->codec = HB_ACODEC_AC3;
                break;

            case 0x02:
            case 0x03:
                audio->id    = 0xc0 + position;
                audio->codec = HB_ACODEC_MPGA;
                break;

            case 0x04:
                audio->id    = ( ( 0xa0 + position ) << 8 ) | 0xbd;
                audio->codec = HB_ACODEC_LPCM;
                break;

            case 0x06:
                audio->id    = ( ( 0x88 + position ) << 8 ) | 0xbd;
                audio->codec = HB_ACODEC_DCA;
                break;

            default:
                audio->id    = 0;
                audio->codec = 0;
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

        lang = lang_for_code( vts->vtsi_mat->vts_audio_attr[i].lang_code );

        snprintf( audio->lang, sizeof( audio->lang ), "%s (%s)",
            strlen(lang->native_name) ? lang->native_name : lang->eng_name,
            audio->codec == HB_ACODEC_AC3 ? "AC3" : ( audio->codec ==
                HB_ACODEC_DCA ? "DTS" : ( audio->codec ==
                HB_ACODEC_MPGA ? "MPEG" : "LPCM" ) ) );
        snprintf( audio->lang_simple, sizeof( audio->lang_simple ), "%s",
                  strlen(lang->native_name) ? lang->native_name : lang->eng_name );
        snprintf( audio->iso639_2, sizeof( audio->iso639_2 ), "%s",
                  lang->iso639_2);

        hb_log( "scan: id=%x, lang=%s, 3cc=%s", audio->id,
                audio->lang, audio->iso639_2 );

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

        lang = lang_for_code( vts->vtsi_mat->vts_subp_attr[i].lang_code );

        subtitle = calloc( sizeof( hb_subtitle_t ), 1 );
        subtitle->id = ( ( 0x20 + position ) << 8 ) | 0xbd;
        snprintf( subtitle->lang, sizeof( subtitle->lang ), "%s",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name);
       snprintf( subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "%s",
                 lang->iso639_2);

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

        if( chapter->block_count < 2048 && c > 1 )
        {
            hb_log( "scan: chapter %d(%d) too short (%d blocks, "
                    "cells=%d->%d), merging", c, chapter->index,
                    chapter->block_count, chapter->cell_start,
                    chapter->cell_end );
            chapter_old = hb_list_item( title->list_chapter, c - 2 );
            chapter_old->cell_end    = chapter->cell_end;
            chapter_old->block_end   = chapter->block_end;
            chapter_old->block_count += chapter->block_count;
            chapter_old->duration += chapter->duration;
            free( chapter );
            chapter = chapter_old;
        }
        else
        {
            hb_list_add( title->list_chapter, chapter );
            c++;
        }
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
            title->aspect = HB_ASPECT_BASE * 4 / 3;
            break;
        case 3:
            title->aspect = HB_ASPECT_BASE * 16 / 9;
            break;
        default:
            hb_log( "scan: unknown aspect" );
            goto fail;
    }

    hb_log( "scan: aspect = %d", title->aspect );

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

    return 1;
}


/***********************************************************************
 * is_nav_pack
 ***********************************************************************
 * Pretty much directly lifted from libdvdread's play_title function.
 **********************************************************************/
int is_nav_pack( unsigned char *buf )
{
    return ( buf[41] == 0xbf && buf[1027] == 0xbf );
}


/***********************************************************************
 * hb_dvd_read
 ***********************************************************************
 *
 **********************************************************************/
int hb_dvd_read( hb_dvd_t * d, hb_buffer_t * b )
{
    if( !d->pack_len )
    {
        /* New pack */
        dsi_t dsi_pack;
        int   error;

        error = 0;
        
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
                hb_log( "dvd: Unrecoverable Read Error from DVD, potential HD or DVD Failure (blk: %d)", d->next_vobu );
                return 0;
            }

            if ( !is_nav_pack( b->data ) ) { 
                (d->next_vobu)++;
                continue;
            }

            navRead_DSI( &dsi_pack, &b->data[DSI_START_BYTE] );
            
            block     = dsi_pack.dsi_gi.nv_pck_lbn;
            pack_len  = dsi_pack.dsi_gi.vobu_ea;
            next_vobu = block + ( dsi_pack.vobu_sri.next_vobu & 0x7fffffff );

            if( pack_len >  0    &&
                pack_len <  1024 &&
                block    >= d->next_vobu &&
                ( block <= d->title_start + d->title_block_count ||
                  block <= d->title_end ) )
            {
                /* XXX
                   This looks like a valid VOBU, but actually we are
                   just hoping */
                if( error )
                {
#if 0
                    hb_log( "dvd: found VOBU at %d (b %d, l %d, n %d)",
                            d->next_vobu, block, pack_len, next_vobu );
#endif
                }
                d->block     = block;
                d->pack_len  = pack_len;
                d->next_vobu = next_vobu;
                break;
            }

            /* Wasn't a valid VOBU, try next block */
            if( !error )
            {
#if 0
                hb_log( "dvd: looking for a VOBU (%d)", d->next_vobu );
#endif
            }

            if( ++error > 1024 )
            {
                hb_log( "dvd: couldn't find a VOBU after 1024 blocks" );
                return 0;
            }

            (d->next_vobu)++;
        }

        if( dsi_pack.vobu_sri.next_vobu == SRI_END_OF_CELL )
        {
            hb_log( "DVD: End of Cell (%d) at block %d", d->cell_cur, 
                    d->block );
            d->cell_cur  = d->cell_next;
            d->next_vobu = d->pgc->cell_playback[d->cell_cur].first_sector;
            FindNextCell( d );
            d->cell_overlap = 1;
        }
        
        // Revert the cell overlap, and check for a chapter break
        if( dsi_pack.vobu_sri.prev_vobu == SRI_END_OF_CELL )
        {
            hb_log( "DVD: Beginning of Cell (%d) at block %d", d->cell_cur, 
                   d->block );
            if( d->cell_overlap )
            {
                b->new_chap = hb_dvd_is_break( d );
                d->cell_overlap = 0;
            }
        }
    }
    else
    {
        if( DVDReadBlocks( d->file, d->block, 1, b->data ) != 1 )
        {
            hb_error( "reader: DVDReadBlocks failed (%d)", d->block );
            return 0;
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
 * Returns 1 if the current block is a new chapter start
 **********************************************************************/
int hb_dvd_is_break( hb_dvd_t * d )
{
    int     i, j;
    int     pgc_id, pgn;
	int     nr_of_ptts = d->ifo->vts_ptt_srpt->title[d->ttn-1].nr_of_ptts;
    pgc_t * pgc;
    int     cell, chapter_length, cell_end;
    
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
            /* Check to see if we merged this chapter into the previous one... */
            /* As a note, merging chapters is probably bad practice for this very reason */
            chapter_length = 0;
            
            if( i == nr_of_ptts - 1 )
            {
                cell_end = d->pgc->nr_of_cells;
            }
            else
            {
                cell_end = pgc->program_map[pgn] - 1;
            }
            
            for( j = cell; j < cell_end; j++ )
            {
                chapter_length += pgc->cell_playback[j].last_sector + 1 - 
                                  pgc->cell_playback[j].first_sector;
            }
            
            if( chapter_length >= 2048 )
            {
                hb_log("DVD: Chapter Break Cell Found");
                /* We have a chapter break */
                return 1;
            }
            else
            {
                hb_log("DVD: Cell Found (%d)", chapter_length);
            }
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
