/* $Id: lang.h,v 1.1 2004/08/02 07:19:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_LANG_H
#define HB_LANG_H

typedef struct iso639_lang_t
{
    char * eng_name;        /* Description in English */
    char * native_name;     /* Description in native language */
    char * iso639_1;       /* ISO-639-1 (2 characters) code */
    char * iso639_2;        /* ISO-639-2 (3 character) code */

} iso639_lang_t;

#ifdef __cplusplus
extern "C" {
#endif
iso639_lang_t * lang_for_code( int code );

iso639_lang_t * lang_for_english( const char * english );
#ifdef __cplusplus
}
#endif
#endif
