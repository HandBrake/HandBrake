/* bits.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_BITS_H
#define HB_BITS_H

static inline int
allbits_set(uint32_t *bitmap, int num_words)
{
    unsigned int i;
    for( i = 0; i < num_words; i++ )
    {
        if( bitmap[i] != 0xFFFFFFFF )
            return (0);
    }
    return (1);
}

static inline int
bit_is_set( uint32_t *bit_map, int bit_pos )
{
    return( ( bit_map[bit_pos >> 5] & (0x1 << (bit_pos & 0x1F) ) ) != 0 );
}

static inline int
bit_is_clear( uint32_t *bit_map, int bit_pos )
{
    return( ( bit_map[bit_pos >> 5] & ( 0x1 << (bit_pos & 0x1F) )  ) == 0 );
}

static inline void
bit_set( uint32_t *bit_map, int bit_pos )
{
    bit_map[bit_pos >> 5] |= 0x1 << (bit_pos & 0x1F);
}

static inline void
bit_clear(uint32_t *bit_map, int bit_pos)
{
    bit_map[bit_pos >> 5] &= ~( 0x1 << ( bit_pos & 0x1F ) );
}

static inline void
bit_nclear(uint32_t *bit_map, int start_pos, int stop_pos)
{
    int start_word = start_pos >> 5;
    int stop_word  = stop_pos >> 5;

    if ( start_word == stop_word )
    {

        bit_map[start_word] &= ( ( 0x7FFFFFFF >> ( 31 - (start_pos & 0x1F ) ) )
                             |  ( 0xFFFFFFFE << ( stop_pos & 0x1F ) ) );
    }
    else
    {
        bit_map[start_word] &= ( 0x7FFFFFFF >> ( 31 - ( start_pos & 0x1F ) ) );
        while (++start_word < stop_word)
            bit_map[start_word] = 0;
        bit_map[stop_word]  &= 0xFFFFFFFE << ( stop_pos & 0x1F );
    }
}

static inline void
bit_nset(uint32_t *bit_map, int start_pos, int stop_pos)
{
    int start_word = start_pos >> 5;
    int stop_word  = stop_pos >> 5;

    if ( start_word == stop_word )
    {
        bit_map[start_word] |= ( ( 0xFFFFFFFF << ( start_pos & 0x1F ) )
                             &  ( 0xFFFFFFFF >> ( 31 - ( stop_pos & 0x1F ) ) ) );
    }
    else
    {
        bit_map[start_word] |= 0xFFFFFFFF << ( start_pos & 0x1F );
        while (++start_word < stop_word)
            bit_map[start_word] = 0xFFFFFFFF;
        bit_map[stop_word]  |= 0xFFFFFFFF >> ( 31 - ( stop_pos & 0x1F ) );
    }
}

#endif /* HB_BITS_H */
