/* lang.h

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_LANG_H
#define HB_LANG_H

typedef struct iso639_lang_t
{
    char * eng_name;        /* Description in English */
    char * native_name;     /* Description in native language */
    char * iso639_1;       /* ISO-639-1 (2 characters) code */
    char * iso639_2;        /* ISO-639-2/t (3 character) code */
    char * iso639_2b;       /* ISO-639-2/b code (if different from above) */

} iso639_lang_t;

#ifdef __cplusplus
extern "C" {
#endif
/* find language associated with ISO-639-1 language code */
iso639_lang_t * lang_for_code( int code );

/* find language associated with ISO-639-2 language code */
iso639_lang_t * lang_for_code2( const char *code2 );

/* ISO-639-1 code for language */
int lang_to_code(const iso639_lang_t *lang);

iso639_lang_t * lang_for_english( const char * english );
#ifdef __cplusplus
}
#endif
#endif
