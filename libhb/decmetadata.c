/* decmetadata.c - Extract and decode metadata from the source

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <mp4v2/mp4v2.h>

#include "common.h"

void identify_art_type( hb_metadata_t *metadata )
{
    typedef struct header_s {
        enum arttype type;
        char*   name;   // short string describing name of type
        char*   data;   // header-bytes to match
    } header;

    // types which may be detected by first-bytes only
    static header headers[] = {
        { BMP,    "bmp", "\x4d\x42" },
        { GIF87A, "GIF (87a)", "GIF87a" },
        { GIF89A, "GIF (89a)", "GIF89a" },
        { JPG,    "JPEG", "\xff\xd8\xff\xe0" },
        { PNG,    "PNG", "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a" },
        { TIFFL,  "TIFF (little-endian)", "II42" },
        { TIFFB,  "TIFF (big-endian)", "MM42" },
        { UNKNOWN } // must be last
    };
    header* p;
    header* found = NULL;
    for( p = headers; p->type != UNKNOWN; p++ ) {
        header *h = p;

        if( metadata->coverart_size < strlen(h->data) )
            continue;

        if( memcmp(h->data, metadata->coverart, strlen(h->data)) == 0 ) {
            metadata->coverart_type = h->type;
            break;
        }
    }
}

static void decmp4metadata( hb_title_t *title )
{
    MP4FileHandle input_file;

    hb_deep_log( 2, "Got an MP4 input, read the metadata");

    input_file = MP4Read( title->dvd, 0 );

    if( input_file != MP4_INVALID_FILE_HANDLE )
    { 
        char         *value = NULL;
        uint8_t      *cover_art = NULL;
        uint32_t      size;
        uint32_t      count;
        
        /*
         * Store iTunes MetaData
         */
        if( MP4GetMetadataName( input_file, &value) && value )
        {
            hb_deep_log( 2, "Metadata Name in input file is '%s'", value);
            strncpy( title->metadata->name, value, 255);
            MP4Free(value);
            value = NULL;
        }

        if( MP4GetMetadataArtist( input_file, &value) && value )
        {
            strncpy( title->metadata->artist, value, 255);
            MP4Free(value);
            value = NULL;
        }
        
        if( MP4GetMetadataComposer( input_file, &value) && value )
        {
            strncpy( title->metadata->composer, value, 255);
            MP4Free(value);
            value = NULL;
        }

        if( MP4GetMetadataComment( input_file, &value) && value )
        {
            strncpy( title->metadata->comment, value, 1024);
            value = NULL;
        }
        
        if( MP4GetMetadataReleaseDate( input_file, &value) && value )
        {
            strncpy( title->metadata->release_date, value, 255);
            MP4Free(value);
            value = NULL;
        }
        
        if( MP4GetMetadataAlbum( input_file, &value) && value )
        {
            strncpy( title->metadata->album, value, 255);
            MP4Free(value);
            value = NULL;
        }
        
        if( MP4GetMetadataGenre( input_file, &value) && value )
        {
            strncpy( title->metadata->genre, value, 255);
            MP4Free(value);
            value = NULL;
        }
        
        if( MP4GetMetadataCoverArt( input_file, &cover_art, &size, 0) && 
            cover_art )
        {
            title->metadata->coverart = cover_art; 
            title->metadata->coverart_size = size;
            identify_art_type( title->metadata );
            hb_deep_log( 2, "Got some cover art of type %d, size %d", 
                         title->metadata->coverart_type,
                         title->metadata->coverart_size);
        }
        
        /*
         * Handle the chapters. 
         */
        MP4Chapter_t *chapter_list = NULL;
        uint32_t      chapter_count;
        
        MP4GetChapters( input_file, &chapter_list, &chapter_count, 
                        MP4ChapterTypeQt );

        if( chapter_list ) {
            uint i = 1;
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
                hb_deep_log( 2, "Added chapter %i, name='%s', dur=%lld, (%02i:%02i:%02i)", chapter->index, chapter->title, 
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
