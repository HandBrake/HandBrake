/* $Id: update.c,v 1.7 2005/03/26 23:04:14 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#define HB_URL   "handbrake.fr"
#define HB_QUERY "GET /appcast.xml HTTP/1.0\r\nHost: " HB_URL "\r\n\r\n"

typedef struct
{
    int  * build;
    char * version;

} hb_update_t;

static void UpdateFunc( void * );

hb_thread_t * hb_update_init( int * build, char * version )
{
    hb_update_t * data = calloc( sizeof( hb_update_t ), 1 );
    data->build   = build;
    data->version = version;

    return hb_thread_init( "update", UpdateFunc, data,
                           HB_NORMAL_PRIORITY );
}

static void UpdateFunc( void * _data )
{
    hb_update_t * data = (hb_update_t *) _data;

    hb_net_t * net;
    int        ret;
    char       buf[1024];
    char     * cur, * end, * p;
    int        size;
    int        stable, unstable;
    char       stable_str[16], unstable_str[16];
    int        i;

    if( !( net = hb_net_open( HB_URL, 80 ) ) )
    {
        goto error;
    }

    if( hb_net_send( net, HB_QUERY ) < 0 )
    {
        hb_net_close( &net );
        goto error;
    }

    size = 0;
    memset( buf, 0, 1024 );
    for( ;; )
    {
        ret = hb_net_recv( net, &buf[size], sizeof( buf ) - size );
        if( ret < 1 )
        {
            hb_net_close( &net );
            break;
        }
        size += ret;
    }

    cur = buf;
    end = &buf[sizeof( buf )];

    /* Make sure we got it */
    cur += 9;
    if( size < 15 || strncmp( cur, "200 OK", 6 ) )
    {
        /* Something went wrong */
        goto error;
    }
    cur += 6;

    /* Find the end of the headers and the beginning of the content */
    for( ; &cur[3] < end; cur++ )
    {
        if( cur[0] == '\r' && cur[1] == '\n' &&
            cur[2] == '\r' && cur[3] == '\n' )
        {
            cur += 4;
            break;
        }
    }

    if( cur >= end )
    {
        goto error;
    }
	
	
	// FIND THE STABLE VERSION INFORMATION ###################################################
	
	/*
	 * Find the <cli-stable> tag
	 * Scan though each character of the buffer until we find that the first 4 characters of "cur" are "<cli"
	 */

     for(i=0 ; &cur[3] < end; i++, cur++ )
     {
        if( cur[0] == '<' && cur[1] == 'c' && cur[2] == 'l' && cur[3] == 'i' )
         {
            cur += 1;
            break;
         }
		 
		 // If the CLI tag has not been found in the first 510 characters, or the end is reached, something bad happened.
		 if (( i > 510) || ( cur >= end ))
		 {
		 	goto error;
		 }
     }
	 
	if( cur >= end )
    {
        goto error;
    }
	
	/*
	 * Ok, The above code didn't position cur, it only found <cli so we need to shift cur along 11 places.
	 * After which, the next 10 characters are the build number
	 */
    cur += 11;
	
	if( cur >= end )
    {
        goto error;
    }
	
	stable = strtol( cur, &cur, 10 );
	
	if( cur >= end )
    {
        goto error;
    }
	
	/*
	 * The Version number is 2 places after the build, so shift cur, 2 places.
	 * Get all the characters in cur until the point where " is found.
	 */
	cur += 2;
	
	if( cur >= end )
    {
        goto error;
    }
	memset( stable_str, 0, sizeof( stable_str ) );
	for( i = 0;   i < sizeof( stable_str ) - 1 && cur < end && *cur != '"'; i++, cur++ )
	{
		stable_str[i] = *cur;
		
		// If the version number is longer than 7 characters, or the end is reached, something has gone wrong.
		if (( i > 7) || ( cur >= end ))
		{
			goto error;
		}
	}
	
	if( cur >= end )
    {
        goto error;
    }
	
    hb_log( "latest stable: %s, build %d", stable_str, stable );
	
	// END OF STABLE INFO ###################################################
	

	// FIND THE UNSTABLE INFO ###############################################
	/*
	 * Find the <cli-unstable> tag
	 * Scan though each character of the buffer until we find that the first 4 characters of "cur" are "<cli"
	 */

     for(i =0 ; &cur[3] < end; i++, cur++ )
     {
        if( cur[0] == '<' && cur[1] == 'c' && cur[2] == 'l' && cur[3] == 'i' )
         {
            cur += 1;
            break;
         }
		 
		 // If the second CLI tag is more than 25 characters forward, or the end is reached, something went wrong.
		 if (( i > 25) || ( cur >= end ))
		 {
		 	goto error;
		 }
     }
	 
	/*
	 * Now we need to handle the unstable build information
	 * Unstable build number is 29 Characters after the last position used.
	 */
	 
	 cur += 13;
	     
	if( cur >= end )
    {
        goto error;
    } 
	
	 unstable = strtol( cur, &p, 10 );
	 
	if( cur >= end )
    {
        goto error;
    }
	
	/*
	 * Now we need to get the unstable version number.
	 * First move the cur pointer 12 places.
	 * Then iterate over cur until " is found. Thats the end of the version number.
	 */
	cur += 12;
	
	if( cur >= end )
    {
        goto error;
    }
	
	memset( unstable_str, 0, sizeof( unstable_str ) );
	for( i = 0;   i < sizeof( unstable_str ) - 1 && cur < end && *cur != '"'; i++, cur++ )
	{
		unstable_str[i] = *cur;
		
		// If the version number is greater than 7 chars or the end is reached, something went wrong.
		if (( i > 7) || ( cur >= end ))
		{
			goto error;
		}
	}

    hb_log( "latest unstable: %s, build %d", unstable_str, unstable );
	
	// END OF UNSTABLE INFO ###################################################

	/*
	 * Handle the update checking as normal.
	 * This code is unchanged.
	 */
    if( HB_BUILD % 100 )
    {
        /* We are runnning an unstable build */
        if( unstable > HB_BUILD )
        {
            memcpy( data->version, unstable_str, sizeof( unstable_str ) );
            *(data->build) = unstable;
        }
    }
    else
    {
        /* We are runnning an stable build */
        if( stable > HB_BUILD )
        {
            memcpy( data->version, stable_str, sizeof( stable_str ) );
            *(data->build) = stable;
        }
    }

error:
    free( data );
    return;
}

