/* batch.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/lang.h"

struct hb_batch_s
{
    char        * path;
    hb_list_t   * list_file;
    hb_handle_t * h;
};

static int compare_str(const void *a, const void *b)
{
    return strncmp(*(const char**)a, *(const char**)b, PATH_MAX);
}

/***********************************************************************
 * hb_batch_init
 ***********************************************************************
 *
 **********************************************************************/
hb_batch_t * hb_batch_init( hb_handle_t *h, char * path, hb_list_t * exclude_extensions )
{
    hb_batch_t    * d;
    hb_stat_t       sb;
    HB_DIR        * dir;
    struct dirent * entry;
    char          * filename;
    int             count, ii;
    char         ** files;

    if ( hb_stat( path, &sb ) )
        return NULL;

    if ( !S_ISDIR( sb.st_mode ) )
        return NULL;

    dir = hb_opendir(path);
    if ( dir == NULL )
        return NULL;

    // Count the total number of entries
    count = 0;
    while (hb_readdir(dir))
    {
        count++;
    }

    if (count == 0)
    {
        return NULL;
    }

    files = malloc(count * sizeof(char*));
    
    // Excluded Extensions
    int extension_count = hb_list_count(exclude_extensions);
    hb_log("Excluding %i file extension(s) from scan. ", extension_count);
    
    for (int i = 0; i < extension_count; i++ )
    {
        char * file_extension = hb_list_item( exclude_extensions, i );
        hb_log(" - Excluding Extension: %s", file_extension);
    }
    
    // Find all regular files
    ii = 0;
    hb_rewinddir(dir);
    while ( (entry = hb_readdir( dir ) ) )
    {
        filename = hb_strdup_printf( "%s" DIR_SEP_STR "%s", path, entry->d_name );
        
        int excluded = 0;
        for (int i = 0; i < extension_count; i++ )
        {
            const char *file_extension = hb_list_item(exclude_extensions, i);
            if (hb_str_ends_with(filename, file_extension))
            {
                hb_deep_log(2, " -- Excluding File: %s", filename);
                excluded = 1;
            }
        }
        
        if (excluded)
        {
            free(filename);
            continue;
        }
                
        if (hb_stat(filename, &sb))
        {
            free(filename);
            continue;
        }

        if (!S_ISREG(sb.st_mode))
        {
            free(filename);
            continue;
        }

        files[ii++] = filename;
    }
    count = ii;

    // Sort the files
    qsort(files, count, sizeof(char*), compare_str);

    // Create file list
    d = calloc( sizeof( hb_batch_t ), 1 );
    d->h = h;
    d->list_file = hb_list_init();
    for (ii = 0; ii < count; ii++)
    {
        hb_list_add( d->list_file, files[ii] );
    }
    hb_closedir( dir );
    free(files);

    if ( hb_list_count( d->list_file ) == 0 )
    {
        hb_list_close( &d->list_file );
        free( d );
        return NULL;
    }

    d->path = strdup( path );

    return d;
}

/***********************************************************************
 * hb_batch_title_count
 **********************************************************************/
int hb_batch_title_count( hb_batch_t * d )
{
    return hb_list_count( d->list_file );
}

/***********************************************************************
 * hb_batch_title_scan
 **********************************************************************/
hb_title_t * hb_batch_title_scan( hb_batch_t * d, int t )
{

    hb_title_t   * title;
    char         * filename;
    hb_stream_t  * stream;

    if ( t < 0 )
        return NULL;

    filename = hb_list_item( d->list_file, t - 1 );
    if ( filename == NULL )
        return NULL;

    hb_log( "batch: scanning %s", filename );
    title = hb_title_init( filename, t );
    stream = hb_stream_open(d->h, filename, title, 1);
    if ( stream == NULL )
    {
        hb_title_close( &title );
        return NULL;
    }

    title = hb_stream_title_scan( stream, title );
    hb_stream_close( &stream );

    return title;
}

hb_title_t * hb_batch_title_scan_single(hb_handle_t *h, char *filename, int title_index)
{
    hb_title_t   *title;
    hb_stream_t  *stream;

    if (title_index < 0)
    {
        return NULL;
    }

    if (!hb_is_valid_batch_path(filename))
    {
        return NULL;
    }

    hb_log("batch: scanning %s", filename);
    title = hb_title_init(filename, title_index);
    
    if (title == NULL)
    {
        return NULL;
    }

    stream = hb_stream_open(h, filename, title, 1);

    if (stream == NULL)
    {
        hb_title_close(&title);
        return NULL;
    }

    title = hb_stream_title_scan(stream, title);
    hb_stream_close(&stream);

    return title;
}

int hb_is_valid_batch_path(const char *filename)
{
    hb_stat_t sb;

    if (hb_stat(filename, &sb))
    {
        return 0;
    }

    if (S_ISDIR(sb.st_mode))
    {
        return 0;
    }

    if (!S_ISREG(sb.st_mode))
    {
        return 0;
    }

    return 1;
}

/***********************************************************************
 * hb_batch_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_batch_close( hb_batch_t ** _d )
{
    hb_batch_t * d = *_d;
    char       * filename;

    while ( ( filename = hb_list_item( d->list_file, 0 ) ) )
    {
        hb_list_rem( d->list_file, filename );
        free( filename );
    }
    hb_list_close( &d->list_file );
    free( d->path );
    free( d );
    *_d = NULL;
}

