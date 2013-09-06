/* decmetadata.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */


#include "common.h"

#if defined(USE_MP4V2)
#include <mp4v2/mp4v2.h>

static int decmp4metadata( hb_title_t *title )
{
    MP4FileHandle input_file;
    int result = 0;
    hb_deep_log( 2, "Got an MP4 input, read the metadata");

    input_file = MP4Read( title->path, 0 );

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
            hb_metadata_set_name(title->metadata, tags->name);
            result = 1;
        }

        if( tags->artist )
        {
            hb_metadata_set_artist(title->metadata, tags->artist);
            result = 1;
        }

        if( tags->composer )
        {
            hb_metadata_set_composer(title->metadata, tags->composer);
            result = 1;
        }

        if( tags->comments )
        {
            hb_metadata_set_comment(title->metadata, tags->comments);
            result = 1;
        }

        if( tags->releaseDate )
        {
            hb_metadata_set_release_date(title->metadata, tags->releaseDate);
            result = 1;
        }

        if( tags->album )
        {
            hb_metadata_set_album(title->metadata, tags->album);
            result = 1;
        }

        if( tags->albumArtist )
        {
            hb_metadata_set_album_artist(title->metadata, tags->albumArtist);
            result = 1;
        }

        if( tags->genre )
        {
            hb_metadata_set_genre(title->metadata, tags->genre);
            result = 1;
        }

        if( tags->description )
        {
            hb_metadata_set_description(title->metadata, tags->description);
            result = 1;
        }

        if( tags->longDescription )
        {
            hb_metadata_set_long_description(title->metadata, tags->longDescription);
            result = 1;
        }

        int ii;
        for ( ii = 0; ii < tags->artworkCount; ii++ )
        {
            const MP4TagArtwork* art = tags->artwork + ii;
            int type;
            switch ( art->type )
            {
                case MP4_ART_BMP:
                    type = HB_ART_BMP;
                    break;
                case MP4_ART_GIF:
                    type = HB_ART_GIF;
                    break;
                case MP4_ART_JPEG:
                    type = HB_ART_JPEG;
                    break;
                case MP4_ART_PNG:
                    type = HB_ART_PNG;
                    break;
                default:
                    type = HB_ART_UNDEFINED;
                    break;
            }
            hb_metadata_add_coverart(
                    title->metadata, art->data, art->size, type);
            hb_deep_log( 2, "Got some cover art of type %d, size %d", 
                         art->type,
                         art->size );
            result = 1;
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

                int seconds      = ( chapter->duration + 45000 ) / 90000;
                chapter->hours   = ( seconds / 3600 );
                chapter->minutes = ( seconds % 3600 ) / 60;
                chapter->seconds = ( seconds % 60 );

                if( chapter_list[i-1].title )
                {
                    hb_chapter_set_title( chapter, chapter_list[i-1].title );
                }
                else
                {
                    char chapter_title[80];
                    sprintf( chapter_title, "Chapter %d", chapter->index );
                    hb_chapter_set_title( chapter, chapter_title );
                }

                hb_deep_log( 2, "Added chapter %i, name='%s', dur=%"PRId64", (%02i:%02i:%02i)",
                             chapter->index, chapter->title, chapter->duration,
                             chapter->hours, chapter->minutes, chapter->seconds);

                hb_list_add( title->list_chapter, chapter );
                i++;
            }
        }

        MP4Close( input_file );
    }
    return result;
}
#endif // USE_MP4V2

/*
 * decmetadata()
 *
 * Look at the title and extract whatever metadata we can from that title.
 */
int decmetadata( hb_title_t *title )
{
    if( !title ) 
    {
        return 0;
    }
    
    if( !title->metadata )
    {
        return 0;
    }

#if defined(USE_MP4V2)
    /*
     * Hacky way of figuring out if this is an MP4, in which case read the data using libmp4v2
     */
    if( title->container_name && strcmp(title->container_name, "mov,mp4,m4a,3gp,3g2,mj2") == 0 ) 
    {
        return decmp4metadata( title );
    }
#endif
    return 0;
}
