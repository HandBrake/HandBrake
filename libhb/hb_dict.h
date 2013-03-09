/* hb_dict.h

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#if !defined(HB_DICT_H)
#define HB_DICT_H

typedef struct hb_dict_entry_s hb_dict_entry_t;
typedef struct hb_dict_s       hb_dict_t;

/* Basic dictionary implementation.
 *
 * an hb_dict_t must be initialized with hb_dict_init() before use.
 *
 * "key" must be a string with non-zero length (NULL and "" are invalid keys).
 * "value" can be NULL (the zero-length string "" is mapped to NULL).
 *
 * hb_dict_next( dict, NULL ) returns the first key in the dictionary.
 * hb_dict_next( dict, previous ) returns key directly following previous, or
 * NULL if the end of the dictionary was reached.
 *
 * hb_encopts_to_dict() converts an op1=val1:opt2=val2:opt3=val3 type string to
 * an hb_dict_t dictionary. */

hb_dict_t * hb_dict_init( int alloc );
void        hb_dict_free( hb_dict_t ** dict_ptr );

void hb_dict_set(   hb_dict_t ** dict_ptr, const char * key, const char * value );
void hb_dict_unset( hb_dict_t ** dict_ptr, const char * key );

hb_dict_entry_t * hb_dict_get(  hb_dict_t * dict, const char * key );
hb_dict_entry_t * hb_dict_next( hb_dict_t * dict, hb_dict_entry_t * previous );

hb_dict_t * hb_encopts_to_dict( const char * encopts, int encoder );
char      * hb_dict_to_encopts( hb_dict_t  * dict );

struct hb_dict_entry_s
{
    char * key;
    char * value;
};

struct hb_dict_s
{
    int alloc;
    int count;
    hb_dict_entry_t * objects;
};

#endif // !defined(HB_DICT_H)
