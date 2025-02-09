/* lang.h

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_LANG_H
#define HANDBRAKE_LANG_H

typedef struct iso639_lang_t
{
    char * eng_name;        /* Description in English */
    char * native_name;     /* Description in native language */
    char * iso639_1;        /* ISO-639-1 (2 characters) code */
    char * iso639_2;        /* ISO-639-2/t (3 character) code */
    char * iso639_2b;       /* ISO-639-2/b code (if different from above) */
	char * iso639;			/* Obsolete ISO-639 code (if changed in ISO-639-1) */
} iso639_lang_t;

#ifdef __cplusplus
extern "C" {
#endif
/* find language, match any of names in lang struct */
const iso639_lang_t * lang_lookup( const char * str );

/* find language table index, match any of names in lang struct */
int lang_lookup_index( const char * str );

/* return language for an index into the language table */
const iso639_lang_t * lang_for_index( int index );

/* find language associated with ISO-639-1 language code */
iso639_lang_t * lang_for_code( int code );

/* find language associated with ISO-639-2 language code */
iso639_lang_t * lang_for_code2( const char *code2 );

/* ISO-639-1 code for language */
int lang_to_code(const iso639_lang_t *lang);

iso639_lang_t * lang_for_english( const char * english );

/*
 * Get fake iso639 corresponding to "Any"
 * "Any" is used when a match for any language is desired.
 *
 * Calling lang_get_next() with pointer returned by lang_get_any()
 * returns the first entry in the languages list
 */
const iso639_lang_t* lang_get_any(void);

/*
 * Get the next language in the list.
 * Returns NULL if there are no more languages.
 * Pass NULL to get the first language in the list.
 */
const iso639_lang_t* lang_get_next(const iso639_lang_t *last);

#ifdef __cplusplus
}
#endif
#endif // HANDBRAKE_LANG_H
