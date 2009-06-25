/* decmetadata.c - Extract and decode metadata from the source

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <mp4v2/mp4v2.h>

#include "common.h"

static void decmp4metadata( hb_title_t *title )
{
    MP4FileHandle input_file;
    hb_deep_log( 2, "Got an MP4 input, read the metadata");

    input_file = MP4Read( title->dvd, 0 );

    if( input_file != MP4_INVALID_FILE_HANDLE )
    { 
        /*
         * Store iTunes MetaData
         */
        const MP4Tags* tags;

        /* alloc,fetch tags */
        tags = MP4TagsAlloc();
        MP4TagsFetch( tags, input_file );

        if( tags->name ) {
            hb_deep_log( 2, "Metadata Name in input file is '%s'", tags->name );
            strncpy( title->metadata->name, tags->name, sizeof(title->metadata->name) );
        }

        if( tags->artist )
            strncpy( title->metadata->artist, tags->artist, sizeof(title->metadata->artist) );

        if( tags->composer )
            strncpy( title->metadata->composer, tags->composer, sizeof(title->metadata->composer) );

        if( tags->comments )
            strncpy( title->metadata->comment, tags->comments, sizeof(title->metadata->comment) );

        if( tags->releaseDate )
            strncpy( title->metadata->release_date, tags->releaseDate, sizeof(title->metadata->release_date) );

        if( tags->album )
            strncpy( title->metadata->album, tags->album, sizeof(title->metadata->album) );

        if( tags->genre )
            strncpy( title->metadata->genre, tags->genre, sizeof(title->metadata->genre) );

        if( tags->artworkCount > 0 ) {
            const MP4TagArtwork* art = tags->artwork + 0; // first element
            title->metadata->coverart = (uint8_t*)malloc( art->size );
            title->metadata->coverart_size = art->size;
            memcpy( title->metadata->coverart, art->data, art->size );
            hb_deep_log( 2, "Got some cover art of type %d, size %d", 
                         art->type,
                         title->metadata->coverart_size );
        }

        /* store,free tags */
        MP4TagsStore( tags, input_file );
        MP4TagsFree( tags );
        
        /*
         * Handle the chapters. 
         */
        MP4Chapter_t *chapter_list = NULL;
        uint32_t      chapter_count;
        
        MP4GetChapters( input_file, &chapter_list, &chapter_count, 
                        MP4ChapterTypeQt );

        if( chapter_list && ( hb_list_count( title->list_chapter ) == 0 ) ) {
            uint32_t i = 1;
            while( i <= chapter_count )
            {
                hb_chapter_t * chapter;
                chapter = calloc( sizeof( hb_chapter_t ), 1 );
                chapter->index = i;
                chapter->duration = chapter_list[i-1].duration * 90;
                chapter->hours    = chapter->duration / 90000 / 3600;
                chapter->minutes  = ( ( chapter->duration / 90000 ) % 3600 ) / 60;
                chapter->seconds  = ( chapter->duration / 90000 ) % 60;
                strcpy( chapter->title, chapter_list[i-1].title );
                hb_deep_log( 2, "Added chapter %i, name='%s', dur=%"PRId64", (%02i:%02i:%02i)", chapter->index, chapter->title, 
                       chapter->duration, chapter->hours, 
                       chapter->minutes, chapter->seconds);
                hb_list_add( title->list_chapter, chapter );
                i++;
            }
        }

        MP4Close( input_file );
    }
}

/*
 * decmetadata()
 *
 * Look at the title and extract whatever metadata we can from that title.
 */
void decmetadata( hb_title_t *title )
{
    if( !title ) 
    {
        return;
    }
    
    if( title->metadata )
    {
        free( title->metadata );
        title->metadata = NULL;
    }

    title->metadata = calloc( sizeof(hb_metadata_t), 1);

    if( !title->metadata )
    {
        return;
    }

    /*
     * Hacky way of figuring out if this is an MP4, in which case read the data using libmp4v2
     */
    if( title->container_name && strcmp(title->container_name, "mov,mp4,m4a,3gp,3g2,mj2") == 0 ) 
    {
        decmp4metadata( title );
    } else {
        free( title->metadata );
        title->metadata = NULL;
    }
}
