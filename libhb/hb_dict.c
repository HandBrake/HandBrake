/* This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "hb_dict.h"

hb_dict_t * hb_dict_init( int alloc )
{
    hb_dict_t * dict = NULL;
    dict = malloc( sizeof( hb_dict_t ) );
    if( !dict )
    {
        hb_log( "ERROR: could not allocate hb_dict_t" );
        return NULL;
    }
    dict->count = 0;
    dict->objects = malloc( alloc * sizeof( hb_dict_entry_t ) );
    if( !dict->objects )
    {
        hb_log( "ERROR: could not allocate hb_dict_t objects" );
        dict->alloc = 0;
    }
    else
    {
        dict->alloc = alloc;
    }
    return dict;
}

void hb_dict_free( hb_dict_t ** dict_ptr )
{
    hb_dict_t * dict = *dict_ptr;
    if( dict )
    {
        if( dict->objects )
        {
            int i;
            for( i = 0; i < dict->count; i++ )
            {
                if( dict->objects[i].key )
                {
                    free( dict->objects[i].key );
                }
                if( dict->objects[i].value )
                {
                    free( dict->objects[i].value );
                }
            }
            free( dict->objects );
        }
        free( *dict_ptr );
        *dict_ptr = NULL;
    }
}

void hb_dict_set( hb_dict_t ** dict_ptr, const char * key, const char * value )
{
    hb_dict_t * dict = *dict_ptr;
    if( !dict )
    {
        hb_log( "hb_dict_set: NULL dictionary" );
        return;
    }
    if( !key || !strlen( key ) )
        return;
    hb_dict_entry_t * entry = hb_dict_get( dict, key );
    if( entry )
    {
        if( entry->value )
        {
            if( value && !strcmp( value, entry->value ) )
                return;
            else
            {
                free( entry->value );
                entry->value = NULL;
            }
        }
        if( value && strlen( value ) )
            entry->value = strdup( value );
    }
    else
    {
        if( dict->alloc <= dict->count )
        {
            hb_dict_entry_t * tmp = NULL;
            tmp = malloc( ( 2 * dict->alloc ) * sizeof( hb_dict_entry_t ) );
            if( !tmp )
            {
                hb_log( "ERROR: could not realloc hb_dict_t objects" );
                return;
            }
            if( dict->objects )
            {
                if( dict->count )
                    memcpy( tmp, dict->objects, dict->count * sizeof( hb_dict_entry_t ) );
                free( dict->objects );
            }
            dict->objects = tmp;
            dict->alloc *= 2;
        }
        dict->objects[dict->count].key = strdup( key );
        if( value && strlen( value ) )
            dict->objects[dict->count].value = strdup( value );
        else
            dict->objects[dict->count].value = NULL;
        dict->count++;
    }
}

hb_dict_entry_t * hb_dict_get( hb_dict_t * dict, const char * key )
{
    if( !dict || !dict->objects || !key || !strlen( key ) )
        return NULL;
    int i;
    for( i = 0; i < dict->count; i++ )
        if( !strcmp( key, dict->objects[i].key ) )
            return &dict->objects[i];
    return NULL;
}

hb_dict_entry_t * hb_dict_next( hb_dict_t * dict, hb_dict_entry_t * previous )
{
    if( !dict || !dict->objects )
        return NULL;
    if( !previous )
        return &dict->objects[0];
    unsigned int prev_index = previous - dict->objects;
    if( prev_index + 1 < dict->count )
            return &dict->objects[prev_index+1];
    return NULL;
}

hb_dict_t * hb_encopts_to_dict( const char * encopts )
{
    hb_dict_t * dict = NULL;
    if( encopts && *encopts )
    {
        char *cur_opt, *opts_start, *name, *value;
        dict = hb_dict_init( 10 );
        if( !dict )
            return NULL;
        cur_opt = opts_start = strdup( encopts );
        if( opts_start )
        {
            while( *cur_opt )
            {
                name = cur_opt;
                cur_opt += strcspn( cur_opt, ":" );
                if( *cur_opt )
                {
                    *cur_opt = 0;
                    cur_opt++;
                }
                value = strchr( name, '=' );
                if( value )
                {
                    *value = 0;
                    value++;
                }
                hb_dict_set( &dict, name, value );
            }
        }
        free( opts_start );
    }
    return dict;
}