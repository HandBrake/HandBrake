/* eedi2.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   The EEDI2 interpolator was created by tritical:
   http://web.missouri.edu/~kes25c/
*/

#include "handbrake/handbrake.h"
#include "handbrake/eedi2.h"

/**
 * EEDI2 directional limit lookup table
 *
 * These values are used to limit the range of edge direction searches and filtering.
 */
const int eedi2_limlut[33] __attribute__ ((aligned (16))) = {
                         6, 6, 7, 7, 8, 8, 9, 9, 9, 10,
                         10, 11, 11, 12, 12, 12, 12, 12, 12, 12,
                         12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
                         12, -1, -1 };

/**
 * Analog of _aligned_malloc
 * @param size Size of memory being pointed to
 * @param align_size Size of memory chunks to align to (must be power of 2)
 */
void *eedi2_aligned_malloc( size_t size, size_t align_size )
{
  char * ptr, * ptr2, * aligned_ptr;
  int align_mask = align_size - 1;

  ptr = (char *)malloc( size + align_size + sizeof( int ) );
  if( ptr==NULL ) return( NULL );

  ptr2 = ptr + sizeof( int );
  aligned_ptr = ptr2 + ( align_size - ( (size_t)ptr2 & align_mask ) );


  ptr2 = aligned_ptr - sizeof( int );
  *( (int *)ptr2 ) = (int)( aligned_ptr - ptr );

  return( aligned_ptr );
}

/**
 * Analog of _aligned_free
 * @param ptr The aligned pointer, created with eedi2_aligned_malloc, to be freed
 */
void eedi2_aligned_free( void *ptr )
{
  int * ptr2 = (int *)ptr - 1;
  ptr -= * ptr2;
  free(ptr);
}

/**
 * Sorts metrics for median filtering
 * @param order Pointer to the table of values to sort
 * @param length Length of the order array
 */
void eedi2_sort_metrics( int *order, const int length )
{
    int i;
    for( i = 1; i < length; ++i )
    {
        int j = i;
        const int temp = order[j];
        while( j > 0 && order[j-1] > temp )
        {
            order[j] = order[j-1];
            --j;
        }
        order[j] = temp;
    }
}

#define BIT_DEPTH 8
#include "templates/eedi2_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 16
#include "templates/eedi2_template.c"
#undef BIT_DEPTH
