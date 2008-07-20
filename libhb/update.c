/* $Id: update.c,v 1.7 2005/03/26 23:04:14 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

static void UpdateFunc( void * );

typedef struct
{
    int  * build;
    char * version;

} hb_update_t;

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
	
	char     * hb_query, * hb_query_two;
	hb_net_t * net;
    int        ret;
    char       buf[4096];
    char     * cur, * end;
    int        size;
    int        stable, unstable;
    char       stable_str[16], unstable_str[16];
    int        i;
	
    /* Setup hb_query and hb_query_two with the correct appcast file */
    if( HB_BUILD % 100 )
    {	
        hb_log("Using http://handbrake.fr/appcast_unstable.xml (primary)");
        hb_log("Using http://handbrake.fr/appcast.xml (secondary)");
    }
	else 
	{
        hb_log("Using http://handbrake.fr/appcast.xml (primary)");
        hb_log("Using http://handbrake.fr/appcast_unstable.xml (secondary)");
    }
	
	hb_query = "GET /appcast.xml HTTP/1.0\r\nHost: handbrake.fr\r\n\r\n";
    hb_query_two = "GET /appcast_unstable.xml HTTP/1.0\r\nHost: handbrake.fr\r\n\r\n";

	/* Grab the data from the web server */
    if( !( net = hb_net_open( "handbrake.fr", 80 ) ) )
    {
        goto error;
    }

    if( hb_net_send( net, hb_query ) < 0 )
    {
        hb_log("Error: Unable to connect to server");
        hb_net_close( &net );
        goto error;
    }

    size = 0;
    memset( buf, 0, 4096 );
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
        hb_log("Error: We did not get a 200 OK from the server. \n");
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
        hb_log("Error: Found the end of the buffer before the end of the HTTP header information! \n");
        goto error;
    }
	
    /*
     * Find the <cli> tag
     * Scan though each character of the buffer until we find that the first 4 characters of "cur" are "<cli"
     */
    for(i=0 ; &cur[3] < end; i++, cur++ )
    {
        if( cur[0] == 'c' && cur[1] == 'l' && cur[2] == 'i' && cur[3] == '>' )
        {
            cur += 1;
            break;
        }
		 
        /* If the CLI tag has not been found in the first 768 characters, or the end is reached, something bad happened.*/
        if (( i > 768) || ( cur >= end ))
		{
            hb_log("Error: Did not find the <cli> tag in the expected maximum amount of characters into the file. \n");
            goto error;
		}
    }
	 
    if( cur >= end )
    {
        goto error;
    }
	
    /*
     * Ok, The above code didn't position cur, it only found <cli so we need to shift cur along 3 places.
     * After which, the next 10 characters are the build number
     */
    cur += 3;
	
    if( cur >= end )
    {
        hb_log("Error: Unexpected end of buffer! Could not find the build information. \n");
        goto error;
    }
	
	/* Stable HB_BUILD */
    stable = strtol( cur, &cur, 10 );
		
    if( cur >= end )
    {
     hb_log("Error: Unexpected end of buffer! \n");
        goto error;
    }
	
    /*
     * The Version number is 2 places after the build, so shift cur, 2 places.
     * Get all the characters in cur until the point where " is found.
     */
    cur += 2;
	
    if( cur >= end )
    {
        hb_log("Error: Unexpected end of buffer! Could not get version number. \n");
        goto error;
    }
    memset( stable_str, 0, sizeof( stable_str ) );
    for( i = 0;   i < sizeof( stable_str ) - 1 && cur < end && *cur != '"'; i++, cur++ )
    {
        stable_str[i] = *cur;
		
        /* If the CLI tag has not been found in the first 768 characters, or the end is reached, something bad happened.*/
        if (( i > 7) || ( cur >= end ))
        {
            hb_log("Error: Version number too long, or end of buffer reached. \n");
            goto error;
        }
    }
	
    if( cur >= end )
    {
        goto error;
    }
	
	/* HANDLE THE SECOND APPCAST FILE NOW */
    /* Grab the data from the web server */
    if( !( net = hb_net_open( "handbrake.fr", 80 ) ) )
    {
        goto error;
    }
	
	if( hb_net_send( net, hb_query_two ) < 0 )
    {
        hb_log("Error: Unable to connect to server");
        hb_net_close( &net );
        goto error;
    }

    size = 0;
    memset( buf, 0, 4096 );
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
        hb_log("Error: We did not get a 200 OK from the server. \n");
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
        hb_log("Error: Found the end of the buffer before the end of the HTTP header information! \n");
        goto error;
    }
	
    /*
     * Find the <cli> tag
     * Scan though each character of the buffer until we find that the first 4 characters of "cur" are "<cli"
     */
    for(i=0 ; &cur[3] < end; i++, cur++ )
    {
        if( cur[0] == 'c' && cur[1] == 'l' && cur[2] == 'i' && cur[3] == '>' )
        {
            cur += 1;
            break;
        }
		 
        // If the CLI tag has not been found in the first 768 characters, or the end is reached, something bad happened.
        if (( i > 768) || ( cur >= end ))
		{
            hb_log("Error: Did not find the <cli> tag in the expected maximum amount of characters into the file. \n");
            goto error;
		}
    }
	 
    if( cur >= end )
    {
        goto error;
    }
	
    /*
     * Ok, The above code didn't position cur, it only found <cli so we need to shift cur along 3 places.
     * After which, the next 10 characters are the build number
     */
    cur += 3;
	
    if( cur >= end )
    {
        hb_log("Error: Unexpected end of buffer! Could not find the build information. \n");
        goto error;
    }
	
	/* UnStable HB_BUILD */
    unstable = strtol( cur, &cur, 10 );
		
    if( cur >= end )
    {
     hb_log("Error: Unexpected end of buffer! \n");
        goto error;
    }
	
    /*
     * The Version number is 2 places after the build, so shift cur, 2 places.
     * Get all the characters in cur until the point where " is found.
     */
    cur += 2;
	
    if( cur >= end )
    {
        hb_log("Error: Unexpected end of buffer! Could not get version number. \n");
        goto error;
    }
    memset( unstable_str, 0, sizeof( unstable_str ) );
    for( i = 0;   i < sizeof( unstable_str ) - 1 && cur < end && *cur != '"'; i++, cur++ )
    {
        unstable_str[i] = *cur;
		
        // If the version number is longer than 7 characters, or the end is reached, something has gone wrong.
        if (( i > 7) || ( cur >= end ))
        {
            hb_log("Error: Version number too long, or end of buffer reached. \n");
            goto error;
        }
    }
	
    if( cur >= end )
    {
        goto error;
    }
	
    /* Print the version information */
    hb_log( "latest stable: %s, build %d", stable_str, stable );
	hb_log( "latest unstable: %s, build %d", unstable_str, unstable );
	
    /* Return the build information */
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
