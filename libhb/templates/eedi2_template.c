/* eedi2_template.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   The EEDI2 interpolator was created by tritical:
   http://web.missouri.edu/~kes25c/
*/

#if BIT_DEPTH > 8
#   define pixel  uint16_t
#   define FUNC(name) name##_##16
#   define BPS 2
#else
#   define pixel  uint8_t
#   define FUNC(name) name##_##8
#   define BPS 1
#endif

void FUNC(eedi2_init_limlut)(void **limlut_out, const int depth)
{
    const unsigned shift = depth - 8;

    pixel *limlut = calloc(33, sizeof(pixel));
    for (int i = 0; i < 33; i++)
    {
        limlut[i] = ((pixel)eedi2_limlut[i]) << shift;
    }
    *limlut_out = limlut;
}

/**
 * Bitblits an image plane (overwrites one bitmap with another)
 * @param dtsp Pointer to destination bitmap
 * @param dst_pitch Stride of destination bitmap
 * @param srcp Pointer to source bitmap
 * @param src_pitch Stride of destination bitmap
 * @param row_size Width of the bitmap being copied
 * @param height Height of the source bitmap
 *
 * When row_size, dst_pitch, and src_pitch are equal, eedi2_bit_blit can work more quickly by copying the whole plane at once instead of individual lines.
 */
void FUNC(eedi2_bit_blit)(pixel *dstp, const int dst_pitch,
                          const pixel *srcp, const int src_pitch,
                          const int row_size, const int height)
{
    if ((!height) || (!row_size))
    {
        return;
    }

    if (height == 1 || (dst_pitch == src_pitch && src_pitch == row_size))
    {
        memcpy(dstp, srcp, row_size * height * BPS);
    }
    else
    {
        for (int y = height; y > 0; --y )
        {
            memcpy(dstp, srcp, row_size * BPS);
            dstp += dst_pitch;
            srcp += src_pitch;
        }
    }
}

/**
 * A specialized variant of bit_blit, just for setting up the initial, field-sized bitmap planes that EEDI2 interpolates from.
 * @param src Pointer to source bitmap plane being copied from
 * @param dst Pointer to the destination bitmap plane being copied to
 * @param pitch Stride of both bitmaps
 * @param height Height of the original, full-size src plane being copied from
 */
void FUNC(eedi2_fill_half_height_buffer_plane)(const pixel *src, pixel *dst, const int pitch, const int height)
{
    /* When TFF, we want to copy alternating
       lines starting at 0, the top field.
       When BFF, we want to start at line 1. */
    for (int y = height; y > 0; y = y - 2)
    {
      memcpy(dst, src, pitch * BPS);
      dst += pitch;
      src += pitch * 2;
    }
}

/**
 * A specialized variant of bit_blit, just for resizing the field-height maps EEDI2 generates to frame-height...a simple line doubler
 * @param srcp Pointer to source bitmap plane being copied from
 * @param dstp Pointer to the destination bitmap plane being copied to
 * @param height Height of the input, half-size src plane being copied from
 * @param pitch Stride of both bitmaps
 */
void FUNC(eedi2_upscale_by_2)(const pixel *srcp, pixel *dstp, const int height, const int pitch)
{
    for (int y = height; y > 0; y--)
    {
      memcpy(dstp, srcp, pitch * BPS);
      dstp += pitch;
      memcpy(dstp, srcp, pitch * BPS);
      srcp += pitch;
      dstp += pitch;
    }
}

/**
 * Finds places where vertically adjacent pixels abruptly change in intensity, i.e., sharp edges.
 * @param dstp Pointer to the destination bitmap
 * @param dst_pitch Stride of dstp
 * @param srcp Pointer to the source bitmap
 * @param src_pitch Stride of srcp
 * @param mtresh Magnitude threshold, ensures it doesn't mark edges on pixels that are too similar (10 is a good default value)
 * @param vthresh Variance threshold, ensures it doesn't look for edges in highly random pixel blocks (20 is a good default value)
 * @param lthresh Laplacian threshold, ensures edges are still prominent in the 2nd spatial derivative of the srcp plane (20 is a good default value)
 * @param height Height of half-height single-field frame
 * @param width Width of srcp bitmap rows, as opposed to the padded stride in src_pitch
 */
void FUNC(eedi2_build_edge_mask)(pixel *dstp, const int dst_pitch, const pixel *srcp, const int src_pitch,
                                 int mthresh, const int lthresh, int vthresh, const int height, const int width, const int depth)
{
    const pixel peak = (1 << depth) - 1;
    const pixel shift = depth - 8;
    const pixel ten = 10 << shift;

    mthresh = mthresh * 10;
    vthresh = vthresh * 81;

    memset(dstp, 0, (height / 2) * dst_pitch * BPS);

    srcp += src_pitch;
    dstp += dst_pitch;
    const pixel *srcpp = srcp-src_pitch;
    const pixel *srcpn = srcp+src_pitch;
    for (int y = 1; y < height - 1; ++y )
    {
        for (int x = 1; x < width-1; ++x )
        {
            if ((abs(srcpp[x]  -   srcp[x]) < ten &&
                 abs( srcp[x]  -  srcpn[x]) < ten &&
                 abs(srcpp[x]  -  srcpn[x]) < ten)
              ||
                (abs(srcpp[x-1] -  srcp[x-1]) < ten &&
                 abs( srcp[x-1] - srcpn[x-1]) < ten &&
                 abs(srcpp[x-1] - srcpn[x-1]) < ten &&
                 abs(srcpp[x+1] -  srcp[x+1]) < ten &&
                 abs( srcp[x+1] - srcpn[x+1]) < ten &&
                 abs(srcpp[x+1] - srcpn[x+1]) < ten))
                continue;

            const int sum = (srcpp[x-1] + srcpp[x] + srcpp[x+1] +
                             srcp[x-1] +  srcp[x]+   srcp[x+1] +
                            srcpn[x-1] + srcpn[x] + srcpn[x+1]) >> shift;

            const int sumsq = (srcpp[x-1] >> shift) * (srcpp[x-1] >> shift) +
                              (srcpp[x]   >> shift) * (srcpp[x]   >> shift) +
                              (srcpp[x+1] >> shift) * (srcpp[x+1] >> shift) +
                              ( srcp[x-1] >> shift) * ( srcp[x-1] >> shift) +
                              ( srcp[x]   >> shift) * ( srcp[x]   >> shift) +
                              ( srcp[x+1] >> shift) * ( srcp[x+1] >> shift) +
                              (srcpn[x-1] >> shift) * (srcpn[x-1] >> shift) +
                              (srcpn[x]   >> shift) * (srcpn[x]   >> shift) +
                              (srcpn[x+1] >> shift) * (srcpn[x+1] >> shift);

            if (9 * sumsq-sum * sum < vthresh)
            {
                continue;
            }

            const int Ix = (srcp[x+1] - srcp[x-1]) >> shift;
            const int Iy = MAX(MAX(abs(srcpp[x] - srcpn[x]),
                                   abs(srcpp[x] - srcp[x])),
                                abs(srcp[x] - srcpn[x])) >> shift;
            if (Ix * Ix + Iy * Iy >= mthresh)
            {
                dstp[x] = peak;
                continue;
            }

            const int Ixx =  (srcp[x-1] - 2 * srcp[x] +  srcp[x+1]) >> shift;
            const int Iyy = (srcpp[x]   - 2 * srcp[x] + srcpn[x]) >> shift;
            if (abs( Ixx ) + abs(Iyy) >= lthresh)
            {
                dstp[x] = peak;
            }
        }
        dstp += dst_pitch;
        srcpp += src_pitch;
        srcp += src_pitch;
        srcpn += src_pitch;
    }
}

/**
 * Expands and smooths out the edge mask
 * @param mskp Pointer to the source edge mask being read from
 * @param msk_pitch Stride of mskp
 * @param dstp Pointer to the destination to store the dilated edge mask
 * @param dst_pitch Stride of dstp
 * @param dstr Dilation threshold, ensures a pixel is only retained as an edge in dstp if this number of adjacent pixels or greater are also edges in mskp (4 is a good default value)
 * @param height Height of half-height field-sized frame
 * @param width Width of mskp bitmap rows, as opposed to the pdded stride in msk_pitch
 */
void FUNC(eedi2_dilate_edge_mask)(const pixel *mskp, const int msk_pitch, pixel *dstp, const int dst_pitch,
                                  const int dstr, const int height, const int width, const int depth)
{
    const pixel peak = (1 << depth) - 1;

    FUNC(eedi2_bit_blit)( dstp, dst_pitch, mskp, msk_pitch, width, height );

    mskp += msk_pitch;
    const pixel *mskpp = mskp - msk_pitch;
    const pixel *mskpn = mskp + msk_pitch;
    dstp += dst_pitch;
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if (mskp[x] != 0)
            {
                continue;
            }

            int count = 0;
            if (mskpp[x-1] == peak) { ++count; }
            if (mskpp[x]   == peak) { ++count; }
            if (mskpp[x+1] == peak) { ++count; }
            if ( mskp[x-1] == peak) { ++count; }
            if ( mskp[x+1] == peak) { ++count; }
            if (mskpn[x-1] == peak) { ++count; }
            if (mskpn[x]   == peak) { ++count; }
            if (mskpn[x+1] == peak) { ++count; }

            if (count >= dstr)
            {
                dstp[x] = peak;
            }
        }
        mskpp += msk_pitch;
        mskp += msk_pitch;
        mskpn += msk_pitch;
        dstp += dst_pitch;
    }
}

/**
 * Contracts the edge mask
 * @param mskp Pointer to the source edge mask being read from
 * @param msk_pitch Stride of mskp
 * @param dstp Pointer to the destination to store the eroded edge mask
 * @param dst_pitch Stride of dstp
 * @param estr Erosion threshold, ensures a pixel isn't retained as an edge in dstp if fewer than this number of adjacent pixels are also edges in mskp (2 is a good default value)
 * @param height Height of half-height field-sized frame
 * @param width Width of mskp bitmap rows, as opposed to the pdded stride in msk_pitch
 */
void FUNC(eedi2_erode_edge_mask)(const pixel *mskp, const int msk_pitch, pixel *dstp, const int dst_pitch,
                                 const int estr, const int height, const int width, const int depth)
{
    const pixel peak = (1 << depth) - 1;

    FUNC(eedi2_bit_blit)( dstp, dst_pitch, mskp, msk_pitch, width, height );

    mskp += msk_pitch;
    const pixel *mskpp = mskp - msk_pitch;
    const pixel *mskpn = mskp + msk_pitch;
    dstp += dst_pitch;
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( mskp[x] != peak ) continue;

            int count = 0;
            if (mskpp[x-1] == peak) { ++count; }
            if (mskpp[x]   == peak) { ++count; }
            if (mskpp[x+1] == peak) { ++count; }
            if ( mskp[x-1] == peak) { ++count; }
            if ( mskp[x+1] == peak) { ++count; }
            if (mskpn[x-1] == peak) { ++count; }
            if (mskpn[x]   == peak) { ++count; }
            if (mskpn[x+1] == peak) { ++count; }

            if (count < estr) { dstp[x] = 0; }
        }
        mskpp += msk_pitch;
        mskp += msk_pitch;
        mskpn += msk_pitch;
        dstp += dst_pitch;
    }
}

/**
 * Smooths out horizontally aligned holes in the mask
 *
 * If none of the 6 horizontally adjacent pixels are edges, mark the current pixel as not edged.
 * If at least 1 of the 3 on either side are edges, mark the current pixel as an edge.
 *
 * @param mskp Pointer to the source edge mask being read from
 * @param msk_pitch Stride of mskp
 * @param dstp Pointer to the destination to store the smoothed edge mask
 * @param dst_pitch Stride of dstp
 * @param height Height of half-height field-sized frame
 * @param width Width of mskp bitmap rows, as opposed to the pdded stride in msk_pitch
 */
void FUNC(eedi2_remove_small_gaps)(const pixel *mskp, const int msk_pitch, pixel *dstp, const int dst_pitch,
                                   const int height, const int width, const int depth)
{
    const pixel peak = (1 << depth) - 1;

    FUNC(eedi2_bit_blit)(dstp, dst_pitch, mskp, msk_pitch, width, height);

    mskp += msk_pitch;
    dstp += dst_pitch;
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 3; x < width - 3; ++x)
        {
            if (mskp[x])
            {
                if (mskp[x-3]) { continue; }
                if (mskp[x-2]) { continue; }
                if (mskp[x-1]) { continue; }
                if (mskp[x+1]) { continue; }
                if (mskp[x+2]) { continue; }
                if (mskp[x+3]) { continue; }
                dstp[x] = 0;
            }
            else
            {
                if ((mskp[x+1] && (mskp[x-1] || mskp[x-2] || mskp[x-3])) ||
                    (mskp[x+2] && (mskp[x-1] || mskp[x-2])) ||
                    (mskp[x+3] && mskp[x-1]))
                    dstp[x] = peak;
            }
        }
        mskp += msk_pitch;
        dstp += dst_pitch;
    }
}

/**
 * Calculates spatial direction vectors for the edges. This is EEDI2's timesink, and can be thought of as YADIF_CHECK on steroids, as both try to discern which angle a given edge follows
 * @param plane The plane of the image being processed, to know to reduce maxd for chroma planes (HandBrake only works with YUV420 video so it is assumed they are half-height)
 * @param mskp Pointer to the source edge mask being read from
 * @param msk_pitch Stride of mskp
 * @param srcp Pointer to the source image being filtered
 * @param src_pitch Stride of srcp
 * @param dstp Pointer to the destination to store the dilated edge mask
 * @param dst_pitch Stride of dstp
 * @param maxd Maximum pixel distance to search (24 is a good default value)
 * @param nt Noise threshold (50 is a good default value)
 * @param height Height of half-height field-sized frame
 * @param width Width of srcp bitmap rows, as opposed to the pdded stride in src_pitch
 */
void FUNC(eedi2_calc_directions)(const int plane, const pixel *mskp, const int msk_pitch, const pixel *srcp, const int src_pitch,
                                 pixel *dstp, const int dst_pitch, const int maxd, const int nt, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);
    const pixel nt13 = (nt << (depth - 8)) * 13;
    const pixel nt19 = (nt << (depth - 8)) * 19;

    if (depth == 8)
    {
        memset(dstp, 255, dst_pitch * height);
    }
    else
    {
        for (int i = 0; i < dst_pitch * height; i++)
        {
            dstp[i] = peak;
        }
    }
    mskp += msk_pitch;
    dstp += dst_pitch;
    srcp += src_pitch;
    const pixel *src2p = srcp - src_pitch * 2;
    const pixel *srcpp = srcp - src_pitch;
    const pixel *srcpn = srcp + src_pitch;
    const pixel *src2n = srcp + src_pitch * 2;
    const pixel *mskpp = mskp - msk_pitch;
    const pixel *mskpn = mskp + msk_pitch;
    const int maxdt = plane == 0 ? maxd : ( maxd >> 1 );

    for (int y = 1; y < height - 1; ++y )
    {
        for (int x = 1; x < width - 1; ++x )
        {
            if( mskp[x] != peak || ( mskp[x-1] != peak && mskp[x+1] != peak ) )
                continue;
            const int startu = MAX( -x + 1, -maxdt );
            const int stopu = MIN( width - 2 - x, maxdt );
            int minb = MIN( nt13,
                            ( abs( srcp[x] - srcpn[x] ) +
                              abs( srcp[x] - srcpp[x] ) ) * 6 );
            int mina = MIN( nt19,
                            ( abs( srcp[x] - srcpn[x] ) +
                              abs( srcp[x] - srcpp[x] ) ) * 9 );
            int minc = mina;
            int mind = minb;
            int mine = minb;
            int dira = -5000, dirb = -5000, dirc = -5000, dird = -5000, dire = -5000;
            for (int u = startu; u <= stopu; ++u )
            {
                if (y == 1 ||
                      mskpp[x-1+u] == peak || mskpp[x+u] == peak || mskpp[x+1+u] == peak )
                {
                    if( y == height - 2 ||
                        mskpn[x-1-u] == peak || mskpn[x-u] == peak || mskpn[x+1-u] == peak )
                    {
                        const int diffsn = abs(  srcp[x-1] - srcpn[x-1-u] ) +
                                           abs(  srcp[x]   - srcpn[x-u] )   +
                                           abs(  srcp[x+1] - srcpn[x+1-u] );

                        const int diffsp = abs(  srcp[x-1] - srcpp[x-1+u] ) +
                                           abs(  srcp[x]   - srcpp[x+u] )   +
                                           abs(  srcp[x+1] - srcpp[x+1+u] );

                        const int diffps = abs( srcpp[x-1] -  srcp[x-1-u] ) +
                                           abs( srcpp[x]   -  srcp[x-u] )   +
                                           abs( srcpp[x+1] -  srcp[x+1-u] );

                        const int diffns = abs( srcpn[x-1] -  srcp[x-1+u] ) +
                                           abs( srcpn[x]   -  srcp[x+u] )   +
                                           abs( srcpn[x+1] -  srcp[x+1+u] );

                        const int diff = diffsn + diffsp + diffps + diffns;
                        int diffd = diffsp + diffns;
                        int diffe = diffsn + diffps;
                        if( diff < minb )
                        {
                            dirb = u;
                            minb = diff;
                        }
                        if( __builtin_expect( y > 1, 1) )
                        {
                            const int diff2pp = abs( src2p[x-1] - srcpp[x-1-u] ) +
                                            abs( src2p[x]   - srcpp[x-u] )   +
                                            abs( src2p[x+1] - srcpp[x+1-u] );
                            const int diffp2p = abs( srcpp[x-1] - src2p[x-1+u] ) +
                                            abs( srcpp[x]   - src2p[x+u] )   +
                                            abs( srcpp[x+1] - src2p[x+1+u] );
                            const int diffa = diff + diff2pp + diffp2p;
                            diffd += diffp2p;
                            diffe += diff2pp;
                            if( diffa < mina )
                            {
                                dira = u;
                                mina = diffa;
                            }
                        }
                        if( __builtin_expect( y < height-2, 1) )
                        {
                            const int diff2nn = abs( src2n[x-1] - srcpn[x-1+u] ) +
                                                abs( src2n[x]   - srcpn[x+u] )   +
                                                abs( src2n[x+1] - srcpn[x+1+u] );
                            const int diffn2n = abs( srcpn[x-1] - src2n[x-1-u] ) +
                                                abs( srcpn[x]   - src2n[x-u] )   +
                                                abs( srcpn[x+1] - src2n[x+1-u] );
                            const int diffc = diff + diff2nn + diffn2n;
                            diffd += diff2nn;
                            diffe += diffn2n;
                            if( diffc < minc )
                            {
                                dirc = u;
                                minc = diffc;
                            }
                        }
                        if( diffd < mind )
                        {
                            dird = u;
                            mind = diffd;
                        }
                        if( diffe < mine )
                        {
                            dire = u;
                            mine = diffe;
                        }
                    }
                }
            }
            int order[5], k=0;
            if( dira != -5000 ) order[k++] = dira;
            if( dirb != -5000 ) order[k++] = dirb;
            if( dirc != -5000 ) order[k++] = dirc;
            if( dird != -5000 ) order[k++] = dird;
            if( dire != -5000 ) order[k++] = dire;
            if( k > 1 )
            {
                eedi2_sort_metrics( order, k );
                const int mid = ( k & 1 ) ?
                                    order[k>>1] :
                                    ( order[(k-1)>>1] + order[k>>1] + 1 ) >> 1;
                const int tlim = MAX(limlut[abs(mid)] >> 2, 2 );
                int sum = 0, count = 0;
                for(int i = 0; i < k; ++i )
                {
                    if( abs( order[i] - mid ) <= tlim )
                    {
                        ++count;
                        sum += order[i];
                    }
                }
                if( count > 1 )
                    dstp[x] = neutral + ( (int)( (float)sum / (float)count ) << shift2 );
                else
                    dstp[x] = neutral;
            }
            else dstp[x] = neutral;
        }
        mskpp += msk_pitch;
        mskp += msk_pitch;
        mskpn += msk_pitch;
        src2p += src_pitch;
        srcpp += src_pitch;
        srcp += src_pitch;
        srcpn += src_pitch;
        src2n += src_pitch;
        dstp += dst_pitch;
    }
}

/**
 * Filters the edge mask
 * @param mskp Pointer to the source edge mask being read from
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the filtered edge mask
 * @param dst_pitch Stride of dstp
 * @param height Height of half-height field-sized frame
 * @param width Width of mskp bitmap rows, as opposed to the pdded stride in msk_pitch
 */
void FUNC(eedi2_filter_map)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, int dmsk_pitch,
                            pixel *dstp, const int dst_pitch, const int height, const int width, const int depth)
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift = 2 + (depth - 8);
    const int twelve = 12 << shift;

    FUNC(eedi2_bit_blit)( dstp, dst_pitch, dmskp, dmsk_pitch, width, height );

    mskp += msk_pitch;
    dmskp += dmsk_pitch;
    dstp += dst_pitch;

    const pixel *dmskpp = dmskp - dmsk_pitch;
    const pixel *dmskpn = dmskp + dmsk_pitch;

    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if (dmskp[x] == peak || mskp[x] != peak)
            {
                continue;
            }
            int dir = ( dmskp[x] - neutral ) >> 2;
            const int lim = MAX( abs( dir ) * 2, twelve );
            dir >>= shift;
            int ict = 0, icb = 0;
            if (dir < 0)
            {
                const int dirt = MAX(-x, dir);
                for (int j = dirt; j <= 0; ++j)
                {
                    if ((abs(dmskpp[x+j] - dmskp[x]) > lim && dmskpp[x+j] != peak) ||
                        (dmskp[x+j] == peak && dmskpp[x+j] == peak) ||
                        (abs(dmskp[x+j] - dmskp[x]) > lim &&  dmskp[x+j] != peak))
                    {
                        ict = 1;
                        break;
                    }
                }
            }
            else
            {
                const int dirt = MIN( width - x - 1, dir);
                for (int j = 0; j <= dirt; ++j)
                {
                    if( ( abs( dmskpp[x+j] - dmskp[x] ) > lim && dmskpp[x+j] != peak ) ||
                        ( dmskp[x+j] == peak && dmskpp[x+j] == peak ) ||
                        ( abs(  dmskp[x+j] - dmskp[x] ) > lim &&  dmskp[x+j] != peak ) )
                    {
                        ict = 1;
                        break;
                    }
                }
            }
            if (ict)
            {
                if (dir < 0)
                {
                    const int dirt = MIN(width - x - 1, abs(dir));
                    for (int j = 0; j <= dirt; ++j )
                    {
                        if( ( abs( dmskpn[x+j] - dmskp[x] ) > lim && dmskpn[x+j] != peak ) ||
                            ( dmskpn[x+j] == peak && dmskp[x+j] == peak ) ||
                            ( abs(  dmskp[x+j] - dmskp[x] ) > lim &&  dmskp[x+j] != peak ) )
                        {
                            icb = 1;
                            break;
                        }
                    }
                }
                else
                {
                    const int dirt = MAX(-x, -dir);
                    for (int j = dirt; j <= 0; ++j)
                    {
                        if( ( abs( dmskpn[x+j] - dmskp[x] ) > lim && dmskpn[x+j] != peak ) ||
                            ( dmskpn[x+j] == peak && dmskp[x+j] == peak ) ||
                            ( abs(  dmskp[x+j] - dmskp[x] ) > lim &&  dmskp[x+j] != peak ) )
                        {
                            icb = 1;
                            break;
                        }
                    }
                }
                if( icb )
                    dstp[x] = peak;
            }
        }
        mskp += msk_pitch;
        dmskpp += dmsk_pitch;
        dmskp += dmsk_pitch;
        dmskpn += dmsk_pitch;
        dstp += dst_pitch;
    }
}


/**
 * Filters the edge direction mask
 * @param mskp Pointer to the edge mask
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the filtered edge direction mask
 * @param dst_pitch Stride of dstp
 * @param height Height of half_height field-sized frame
 * @param width Width of dmskp bitmap rows, as opposed to the pdded stride in dmsk_pitch
 */
void FUNC(eedi2_filter_dir_map)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, const int dmsk_pitch,
                                 pixel *dstp, const int dst_pitch, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);

    FUNC(eedi2_bit_blit)(dstp, dst_pitch, dmskp, dmsk_pitch, width, height);

    dmskp += dmsk_pitch;
    const pixel *dmskpp = dmskp - dmsk_pitch;
    const pixel *dmskpn = dmskp + dmsk_pitch;
    dstp += dst_pitch;
    mskp += msk_pitch;
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( mskp[x] != peak ) continue;
            int u = 0, order[9];
            if( dmskpp[x-1] != peak ) order[u++] = dmskpp[x-1];
            if( dmskpp[x]   != peak ) order[u++] = dmskpp[x];
            if( dmskpp[x+1] != peak ) order[u++] = dmskpp[x+1];
            if(  dmskp[x-1] != peak ) order[u++] =  dmskp[x-1];
            if(  dmskp[x]   != peak ) order[u++] =  dmskp[x];
            if(  dmskp[x+1] != peak ) order[u++] =  dmskp[x+1];
            if( dmskpn[x-1] != peak ) order[u++] = dmskpn[x-1];
            if( dmskpn[x]   != peak ) order[u++] = dmskpn[x];
            if( dmskpn[x+1] != peak ) order[u++] = dmskpn[x+1];
            if( u < 4 )
            {
                dstp[x] = peak;
                continue;
            }
            eedi2_sort_metrics( order, u );
            const int mid = ( u & 1 ) ?
                order[u>>1] : ( order[(u-1)>>1] + order[u>>1] + 1 ) >> 1;
            int sum = 0, count = 0;
            const int lim = limlut[abs(mid-neutral)>>shift2];
            for (int i = 0; i < u; ++i)
            {
                if( abs( order[i] - mid ) <= lim )
                {
                    ++count;
                    sum += order[i];
                }
            }
            if (count < 4 || (count < 5 && dmskp[x] == peak))
            {
                dstp[x] = peak;
                continue;
            }
            dstp[x] = (int)( ( (float)( sum + mid ) / (float)( count + 1 ) ) + 0.5f );
        }
        dmskpp += dmsk_pitch;
        dmskp += dmsk_pitch;
        dmskpn += dmsk_pitch;
        dstp += dst_pitch;
        mskp += msk_pitch;
    }
}

/**
 * Smoothes out the edge direction map
 * @param mskp Pointer to the edge mask
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the expanded edge direction mask
 * @param dst_pitch Stride of dstp
 * @param height Height of half-height field-sized frame
 * @param width Width of dmskp bitmap rows, as opposed to the pdded stride in dmsk_pitch
 */
void FUNC(eedi2_expand_dir_map)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, const int dmsk_pitch,
                                 pixel *dstp, const int dst_pitch, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);

    FUNC(eedi2_bit_blit)( dstp, dst_pitch, dmskp, dmsk_pitch, width, height );

    dmskp += dmsk_pitch;
    const pixel *dmskpp = dmskp - dmsk_pitch;
    const pixel *dmskpn = dmskp + dmsk_pitch;
    dstp += dst_pitch;
    mskp += msk_pitch;
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( dmskp[x] != peak || mskp[x] != peak ) continue;
            int u = 0, order[9];
            if( dmskpp[x-1] != peak ) order[u++] = dmskpp[x-1];
            if( dmskpp[x]   != peak ) order[u++] = dmskpp[x];
            if( dmskpp[x+1] != peak ) order[u++] = dmskpp[x+1];
            if(  dmskp[x-1] != peak ) order[u++] =  dmskp[x-1];
            if(  dmskp[x+1] != peak ) order[u++] =  dmskp[x+1];
            if( dmskpn[x-1] != peak ) order[u++] = dmskpn[x-1];
            if( dmskpn[x]   != peak ) order[u++] = dmskpn[x];
            if( dmskpn[x+1] != peak ) order[u++] = dmskpn[x+1];
            if( u < 5 ) continue;
            eedi2_sort_metrics( order, u );
            const int mid = ( u & 1 ) ?
                order[u>>1] : ( order[(u-1)>>1] + order[u>>1] + 1 ) >> 1;
            int sum = 0, count = 0;
            const int lim = limlut[abs(mid-neutral)>>shift2];
            for (int  i = 0; i < u; ++i)
            {
                if (abs( order[i] - mid ) <= lim)
                {
                    ++count;
                    sum += order[i];
                }
            }
            if (count < 5) continue;
            dstp[x] = (int)( ( (float)( sum + mid ) / (float)( count + 1 ) ) + 0.5f );
        }
        dmskpp += dmsk_pitch;
        dmskp += dmsk_pitch;
        dmskpn += dmsk_pitch;
        dstp += dst_pitch;
        mskp += msk_pitch;
    }
}

/**
 * Re-draws a clearer, less blocky frame-height edge direction mask
 * @param mskp Pointer to the edge mask
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the redrawn direction mask
 * @param dst_pitch Stride of dstp
 * @param tff Whether or not the frame parity is Top Field First
 * @param height Height of the full-frame output
 * @param width Width of dmskp bitmap rows, as opposed to the pdded stride in dmsk_pitch
 */
void FUNC(eedi2_mark_directions_2x)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, const int dmsk_pitch,
                                     pixel *dstp, const int dst_pitch, const int tff, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);

    if (depth == 8)
    {
        memset(dstp, 255, dst_pitch * height);
    }
    else
    {
        for (int i = 0; i < dst_pitch * height; i++)
        {
            dstp[i] = peak;
        }
    }
    dstp  += dst_pitch  * ( 2 - tff );
    dmskp += dmsk_pitch * ( 1 - tff );
    mskp  += msk_pitch  * ( 1 - tff );
    const pixel *dmskpn = dmskp + dmsk_pitch * 2;
    const pixel *mskpn = mskp + msk_pitch * 2;
    for (int y = 2 - tff; y < height - 1; y += 2)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( mskp[x] != peak && mskpn[x] != peak ) continue;
            int v = 0, order[6];
            if(  dmskp[x-1] != peak ) order[v++] = dmskp[x-1];
            if(  dmskp[x]   != peak ) order[v++] = dmskp[x];
            if(  dmskp[x+1] != peak ) order[v++] = dmskp[x+1];
            if( dmskpn[x-1] != peak ) order[v++] = dmskpn[x-1];
            if( dmskpn[x]   != peak ) order[v++] = dmskpn[x];
            if( dmskpn[x+1] != peak ) order[v++] = dmskpn[x+1];
            if( v < 3 ) continue;
            else
            {
                eedi2_sort_metrics( order, v );
                const int mid = ( v & 1 ) ? order[v>>1] : ( order[(v-1)>>1] + order[v>>1]+1) >> 1;
                const int lim = limlut[abs(mid-neutral)>>shift2];
                int u = 0;
                if( abs( dmskp[x-1] - dmskpn[x-1] ) <= lim ||
                    dmskp[x-1] == peak || dmskpn[x-1] == peak )
                        ++u;
                if( abs( dmskp[x]   - dmskpn[x] )   <= lim ||
                    dmskp[x]   == peak || dmskpn[x]   == peak )
                        ++u;
                if( abs( dmskp[x+1] - dmskpn[x-1] ) <= lim ||
                    dmskp[x+1] == peak || dmskpn[x+1] == peak)
                        ++u;
                if( u < 2 ) continue;
                int count = 0, sum = 0;
                for (int i = 0; i < v; ++i )
                {
                    if (abs( order[i] - mid ) <= lim)
                    {
                        ++count;
                        sum += order[i];
                    }
                }
                if( count < v - 2 || count < 2 ) continue;
                dstp[x] = (int)( ( (float)( sum + mid ) / (float)( count + 1 ) ) + 0.5f );
            }
        }
        mskp += msk_pitch * 2;
        mskpn += msk_pitch * 2;
        dstp += dst_pitch * 2;
        dmskp += dmsk_pitch * 2;
        dmskpn += dmsk_pitch * 2;
    }
}

/**
 * Filters the frame-height edge direction mask
 * @param mskp Pointer to the edge mask
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the filtered direction mask
 * @param dst_pitch Stride of dstp
 * @param field Field to filter
 * @param height Height of the full-frame output
 * @param width Width of dmskp bitmap rows, as opposed to the pdded stride in dmsk_pitch
 */
void FUNC(eedi2_filter_dir_map_2x)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, int dmsk_pitch,
                                   pixel *dstp, const int dst_pitch, const int field, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);

    FUNC(eedi2_bit_blit)(dstp, dst_pitch, dmskp, dmsk_pitch, width, height);

    dmskp += dmsk_pitch * ( 2 - field );
    const pixel *dmskpp = dmskp - dmsk_pitch * 2;
    const pixel *dmskpn = dmskp + dmsk_pitch * 2;
    mskp += msk_pitch * ( 1 - field );
    const pixel *mskpn = mskp + msk_pitch * 2;
    dstp += dst_pitch * ( 2 - field );
    for (int y = 2 - field; y < height - 1; y += 2)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( mskp[x] != peak && mskpn[x] != peak ) continue;
            int u = 0, order[9];
            if( y > 1 )
            {
                if( dmskpp[x-1] != peak ) order[u++] = dmskpp[x-1];
                if( dmskpp[x]   != peak ) order[u++] = dmskpp[x];
                if( dmskpp[x+1] != peak ) order[u++] = dmskpp[x+1];
            }
            if( dmskp[x-1] != peak ) order[u++] = dmskp[x-1];
            if( dmskp[x]   != peak ) order[u++] = dmskp[x];
            if( dmskp[x+1] != peak ) order[u++] = dmskp[x+1];
            if( y < height - 2 )
            {
                if( dmskpn[x-1] != peak ) order[u++] = dmskpn[x-1];
                if( dmskpn[x]   != peak ) order[u++] = dmskpn[x];
                if( dmskpn[x+1] != peak ) order[u++] = dmskpn[x+1];
            }
            if( u < 4 )
            {
                dstp[x] = peak;
                continue;
            }
            eedi2_sort_metrics( order, u );
            const int mid = ( u & 1 ) ? order[u>>1] : (order[(u-1)>>1] + order[u>>1] + 1 ) >> 1;
            int sum = 0, count = 0;
            const int lim = limlut[abs(mid-neutral)>>shift2];
            for (int  i = 0; i < u; ++i)
            {
                if( abs( order[i] - mid ) <= lim )
                {
                    ++count;
                    sum += order[i];
                }
            }
            if( count < 4 || ( count < 5 && dmskp[x] == peak ) )
            {
                dstp[x] = peak;
                continue;
            }
            dstp[x] = (int)( ( (float)( sum + mid ) / (float)( count + 1 ) ) + 0.5f );
        }
        mskp += msk_pitch * 2;
        mskpn += msk_pitch * 2;
        dmskpp += dmsk_pitch * 2;
        dmskp += dmsk_pitch * 2;
        dmskpn += dmsk_pitch * 2;
        dstp += dst_pitch * 2;
    }
}

/**
 * Smoothes out the frame-height edge direction mask
 * @param mskp Pointer to the edge mask
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the expanded direction mask
 * @param dst_pitch Stride of dstp
 * @param field Field to filter
 * @param height Height of the full-frame output
 * @param width Width of dmskp bitmap rows, as opposed to the pdded stride in dmsk_pitch
 */
void FUNC(eedi2_expand_dir_map_2x)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, const int dmsk_pitch,
                                   pixel *dstp, const int dst_pitch, const int field, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);

    FUNC(eedi2_bit_blit)( dstp, dst_pitch, dmskp, dmsk_pitch, width, height );

    dmskp += dmsk_pitch * ( 2 - field );
    const pixel *dmskpp = dmskp - dmsk_pitch * 2;
    const pixel *dmskpn = dmskp + dmsk_pitch * 2;
    mskp += msk_pitch * ( 1 - field );
    const pixel *mskpn = mskp + msk_pitch * 2;
    dstp += dst_pitch * ( 2 - field );
    for (int y = 2 - field; y < height - 1; y += 2)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( dmskp[x] != peak || ( mskp[x] != peak && mskpn[x] != peak ) ) continue;
            int u = 0, order[9];
            if( y > 1 )
            {
                if( dmskpp[x-1] != peak ) order[u++] = dmskpp[x-1];
                if( dmskpp[x]   != peak ) order[u++] = dmskpp[x];
                if( dmskpp[x+1] != peak ) order[u++] = dmskpp[x+1];
            }
            if( dmskp[x-1] != peak ) order[u++] = dmskp[x-1];
            if( dmskp[x+1] != peak ) order[u++] = dmskp[x+1];
            if( y < height - 2 )
            {
                if( dmskpn[x-1] != peak) order[u++] = dmskpn[x-1];
                if( dmskpn[x]   != peak) order[u++] = dmskpn[x];
                if( dmskpn[x+1] != peak) order[u++] = dmskpn[x+1];
            }
            if( u < 5 ) continue;
            eedi2_sort_metrics( order, u );
            const int mid = ( u & 1 ) ? order[u>>1] : ( order[(u-1)>>1] + order[u>>1] + 1 ) >> 1;
            int sum = 0, count = 0;
            const int lim = limlut[abs(mid-neutral)>>shift2];
            for (int i = 0; i < u; ++i)
            {
                if( abs( order[i] - mid ) <= lim )
                {
                    ++count;
                    sum += order[i];
                }
            }
            if( count < 5 ) continue;
            dstp[x] = (int)( ( (float)( sum + mid ) / (float)( count + 1 ) ) + 0.5f );
        }
        mskp += msk_pitch * 2;
        mskpn += msk_pitch * 2;
        dmskpp += dmsk_pitch * 2;
        dmskp += dmsk_pitch * 2;
        dmskpn += dmsk_pitch * 2;
        dstp += dst_pitch * 2;
    }
}

/**
 * Like the name suggests, this function fills in gaps in the frame-height edge direction mask
 * @param mskp Pointer to the edge mask
 * @param msk_pitch Stride of mskp
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the destination to store the filled-in direction mask
 * @param dst_pitch Stride of dstp
 * @param field Field to filter
 * @param height Height of the full-frame output
 * @param width Width of dmskp bitmap rows, as opposed to the pdded stride in dmsk_pitch
 */
void FUNC(eedi2_fill_gaps_2x)(const pixel *mskp, const int msk_pitch, const pixel *dmskp, const int dmsk_pitch,
                              pixel *dstp, const int dst_pitch, const int field, const int height, const int width, const int depth)
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift = depth - 8;
    const pixel shift2 = 2 + shift;
    const int eight = 8 << shift;
    const int twenty = 20 << shift;
    const int fiveHundred = 500 << shift;

    FUNC(eedi2_bit_blit)( dstp, dst_pitch, dmskp, dmsk_pitch, width, height );

    dmskp += dmsk_pitch * ( 2 - field );
    const pixel *dmskpp = dmskp - dmsk_pitch * 2;
    const pixel *dmskpn = dmskp + dmsk_pitch * 2;
    mskp += msk_pitch * ( 1 - field );
    const pixel *mskpp = mskp - msk_pitch * 2;
    const pixel *mskpn = mskp + msk_pitch * 2;
    const pixel *mskpnn = mskpn + msk_pitch * 2;
    dstp += dst_pitch * ( 2 - field );
    for (int y = 2 - field; y < height - 1; y += 2)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if( dmskp[x] != peak ||
                ( mskp[x] != peak && mskpn[x] != peak ) ) continue;
            int u = x - 1, back = fiveHundred, forward = -fiveHundred;
            while( u )
            {
                if( dmskp[u] != peak )
                {
                    back = dmskp[u];
                    break;
                }
                if( mskp[u] != peak && mskpn[u] != peak ) break;
                --u;
            }
            int v = x + 1;
            while( v < width )
            {
                if( dmskp[v] != peak )
                {
                    forward = dmskp[v];
                    break;
                }
                if( mskp[v] != peak && mskpn[v] != peak ) break;
                ++v;
            }
            int tc = 1, bc = 1;
            int mint = fiveHundred, maxt = -twenty;
            int minb = fiveHundred, maxb = -twenty;
            for (int j = u; j <= v; ++j )
            {
                if( tc )
                {
                    if( y <= 2 || dmskpp[j] == peak || ( mskpp[j] != peak && mskp[j] != peak ) )
                    {
                        tc = 0;
                        mint = maxt = twenty;
                    }
                    else
                    {
                        if( dmskpp[j] < mint ) mint = dmskpp[j];
                        if( dmskpp[j] > maxt ) maxt = dmskpp[j];
                    }
                }
                if( bc )
                {
                    if( y >= height - 3 || dmskpn[j] == peak || ( mskpn[j] != peak && mskpnn[j] != peak ) )
                    {
                        bc = 0;
                        minb = maxb = twenty;
                    }
                    else
                    {
                        if( dmskpn[j] < minb ) minb = dmskpn[j];
                        if( dmskpn[j] > maxb ) maxb = dmskpn[j];
                    }
                }
            }
            if( maxt == -twenty ) maxt = mint = twenty;
            if( maxb == -twenty ) maxb = minb = twenty;
            int thresh = MAX(
                            MAX( MAX( abs( forward - neutral ), abs( back - neutral ) ) >> 2, eight ),
                            MAX( abs( mint - maxt ), abs( minb - maxb ) ) );
            const int flim = MIN(
                                MAX( abs( forward - neutral ), abs( back - neutral ) ) >> shift2,
                                6 );
            if( abs( forward - back ) <= thresh && ( v - u - 1 <= flim || tc || bc ) )
            {
                double step = (double)( forward - back ) / (double)( v - u );
                for (int j = 0; j < v - u - 1; ++j)
                {
                    dstp[u+j+1] = back + (int)( j * step + 0.5 );
                }
            }
        }
        mskpp += msk_pitch * 2;
        mskp += msk_pitch * 2;
        mskpn += msk_pitch * 2;
        mskpnn += msk_pitch * 2;
        dmskpp += dmsk_pitch * 2;
        dmskp += dmsk_pitch * 2;
        dmskpn += dmsk_pitch * 2;
        dstp += dst_pitch * 2;
    }
}

/**
 * Actually renders the output frame, based on the edge and edge direction masks
 * @param plane The plane of the image being processed, to know to reduce a search distance for chroma planes (HandBrake only works with YUV420 video so it is assumed they are half-height)
 * @param dmskp Pointer to the edge direction mask being read from
 * @param dmsk_pitch Stride of dmskp
 * @param dstp Pointer to the line-doubled source field used being filtered in place
 * @param dst_pitch Stride of dstp
 * @param omskp Pointer to the destination to store the output edge mask used for post-processing
 * @param osmk_pitch Stride of omskp
 * @param field Field to filter
 * @nt Noise threshold, (50 is a good default value)
 * @param height Height of the full-frame output
 * @param width Width of dstp bitmap rows, as opposed to the pdded stride in dst_pitch
 */
void FUNC(eedi2_interpolate_lattice)( const int plane, pixel *dmskp, const int dmsk_pitch, pixel *dstp,
                                      const int dst_pitch, pixel *omskp, const int omsk_pitch, const int field, const int nt,
                                      const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift = depth - 8;
    const pixel shift2 = 2 + shift;
    const pixel three = 3 << shift;
    const pixel nine = 9 << shift;
    const pixel nt4 = (nt << (depth - 8)) * 4;
    const pixel nt7 = (nt << (depth - 8)) * 7;
    const pixel nt8 = (nt << (depth - 8)) * 8;

    if (field == 1)
    {
        FUNC(eedi2_bit_blit)( dstp + ( height - 1 ) * dst_pitch,
                  dst_pitch,
                  dstp + ( height - 2 ) * dst_pitch,
                  dst_pitch,
                  width,
                  1 );
    }
    else
    {
        FUNC(eedi2_bit_blit)( dstp,
                  dst_pitch,
                  dstp + dst_pitch,
                  dst_pitch,
                  width,
                  1 );
    }

    dstp += dst_pitch * ( 1 - field );
    omskp += omsk_pitch * ( 1 - field );
    pixel *dstpn = dstp + dst_pitch;
    pixel *dstpnn = dstp + dst_pitch * 2;
    pixel *omskn = omskp + omsk_pitch * 2;
    dmskp += dmsk_pitch * ( 2 - field );
    for (int y = 2 - field; y < height - 1; y += 2)
    {
        for (int x = 0; x < width; ++x)
        {
            int dir = dmskp[x];
            const int lim = limlut[abs(dir-neutral)>>shift2];
            if( dir == peak ||
                ( abs( dmskp[x] - dmskp[x-1] ) > lim &&
                  abs( dmskp[x] - dmskp[x+1] ) > lim ) )
            {
                dstpn[x] = ( dstp[x] + dstpnn[x] + 1 ) >> 1;
                if( dir != peak ) dmskp[x] = neutral;
                continue;
            }
            if( lim < nine )
            {
                const int sum = (  dstp[x-1] +   dstp[x] +   dstp[x+1] +
                                 dstpnn[x-1] + dstpnn[x] + dstpnn[x+1]) >> shift;
                const int sumsq =(  dstp[x-1] >> shift) * (  dstp[x-1] >> shift)+
                                 (  dstp[x]   >> shift) * (  dstp[x]   >> shift)+
                                 (  dstp[x+1] >> shift) * (  dstp[x+1] >> shift)+
                                 (dstpnn[x-1] >> shift) * (dstpnn[x-1] >> shift)+
                                 (dstpnn[x]   >> shift) * (dstpnn[x]   >> shift)+
                                 (dstpnn[x+1] >> shift) * (dstpnn[x+1] >> shift);
                if( 6 * sumsq - sum * sum < 576 )
                {
                    dstpn[x] = ( dstp[x] + dstpnn[x] + 1 ) >> 1;
                    dmskp[x] = peak;
                    continue;
                }
            }
            if( x > 1 && x < width - 2 &&
                ( (   dstp[x] < MAX(   dstp[x-2],   dstp[x-1] ) - three &&
                      dstp[x] < MAX(   dstp[x+2],   dstp[x+1] ) - three &&
                    dstpnn[x] < MAX( dstpnn[x-2], dstpnn[x-1] ) - three &&
                    dstpnn[x] < MAX( dstpnn[x+2], dstpnn[x+1] ) - three )
                ||
                  (   dstp[x] > MIN(   dstp[x-2],   dstp[x-1] ) + three &&
                      dstp[x] > MIN(   dstp[x+2],   dstp[x+1] ) + three &&
                    dstpnn[x] > MIN( dstpnn[x-2], dstpnn[x-1] ) + three &&
                    dstpnn[x] > MIN( dstpnn[x+2], dstpnn[x+1] ) + three ) ) )
            {
                dstpn[x] = ( dstp[x] + dstpnn[x] + 1 ) >> 1;
                dmskp[x] = neutral;
                continue;
            }
            dir = (dir - neutral + (1 << (shift2 - 1))) >> shift2;
            int val = ( dstp[x] + dstpnn[x] + 1 ) >> 1;
            const int startu = ( dir - 2 < 0 ) ?
                        MAX( -x + 1, MAX( dir - 2, -width + 2 + x ) )
                        :
                        MIN(  x - 1, MIN( dir - 2,  width - 2 - x ) );
            const int stopu =  ( dir + 2 < 0 ) ?
                        MAX( -x + 1, MAX( dir + 2, -width + 2 + x ) )
                        :
                        MIN(  x - 1, MIN( dir + 2,  width - 2 - x ) );
            int min = nt8;
            for (int u = startu; u <= stopu; ++u)
            {
                const int diff =
                    abs(   dstp[x-1] - dstpnn[x-u-1] ) +
                    abs(   dstp[x]   - dstpnn[x-u] )   +
                    abs(   dstp[x+1] - dstpnn[x-u+1] ) +
                    abs( dstpnn[x-1] -   dstp[x+u-1] ) +
                    abs( dstpnn[x]   -   dstp[x+u] )   +
                    abs( dstpnn[x+1] -   dstp[x+u+1] );
                if( diff < min &&
                    ( ( omskp[x-1+u] != peak && abs( omskp[x-1+u] - dmskp[x] ) <= lim ) ||
                     (  omskp[x+u]   != peak && abs( omskp[x+u]   - dmskp[x]) <= lim )  ||
                     (  omskp[x+1+u] != peak && abs( omskp[x+1+u] - dmskp[x]) <= lim ) ) &&
                    ( ( omskn[x-1-u] != peak && abs( omskn[x-1-u] - dmskp[x]) <= lim ) ||
                     (  omskn[x-u]   != peak && abs( omskn[x-u]   - dmskp[x]) <= lim ) ||
                     (  omskn[x+1-u] != peak && abs( omskn[x+1-u] - dmskp[x]) <= lim ) ) )
                {
                    const int diff2 =
                        abs( dstp[x+(u>>1)-1] - dstpnn[x-(u>>1)-1] ) +
                        abs( dstp[x+(u>>1)]   - dstpnn[x-(u>>1)]   ) +
                        abs( dstp[x+(u>>1)+1] - dstpnn[x-(u>>1)+1] );
                    if( diff2 < nt4 &&
                        ( ( ( abs( omskp[x+(u>>1)] - omskn[x-(u>>1)]     ) <= lim ||
                              abs( omskp[x+(u>>1)] - omskn[x-((u+1)>>1)] ) <= lim ) &&
                            omskp[x+(u>>1)] != peak )
                          ||
                          ( ( abs( omskp[x+((u+1)>>1)] - omskn[x-(u>>1)] )     <= lim ||
                              abs( omskp[x+((u+1)>>1)] - omskn[x-((u+1)>>1)] ) <= lim ) &&
                            omskp[x+((u+1)>>1)] != peak ) ) )
                    {
                        if( ( abs( dmskp[x] - omskp[x+(u>>1)] )     <= lim ||
                              abs( dmskp[x] - omskp[x+((u+1)>>1)] ) <= lim ) &&
                            ( abs( dmskp[x] - omskn[x-(u>>1)] )     <= lim ||
                              abs( dmskp[x] - omskn[x-((u+1)>>1)] ) <= lim ) )
                        {
                            val = (   dstp[x+(u>>1)] +   dstp[x+((u+1)>>1)] +
                                    dstpnn[x-(u>>1)] + dstpnn[x-((u+1)>>1)] + 2 ) >> 2;
                            min = diff;
                            dir = u;
                        }
                    }
                }
            }
            if( min != nt8 )
            {
                dstpn[x] = val;
                dmskp[x] = neutral + (dir << shift2);
            }
            else
            {
                const int minm = MIN( dstp[x], dstpnn[x] );
                const int maxm = MAX( dstp[x], dstpnn[x] );
                const int d = plane == 0 ? 4 : 2;
                const int startu = MAX( -x + 1, -d );
                const int stopu = MIN( width - 2 - x, d );
                min = nt7;
                for (int u = startu; u <= stopu; ++u)
                {
                    const int p1 =   dstp[x+(u>>1)] +   dstp[x+((u+1)>>1)];
                    const int p2 = dstpnn[x-(u>>1)] + dstpnn[x-((u+1)>>1)];
                    const int diff =
                        abs(   dstp[x-1] - dstpnn[x-u-1] ) +
                        abs(   dstp[x]   - dstpnn[x-u] )   +
                        abs(   dstp[x+1] - dstpnn[x-u+1] ) +
                        abs( dstpnn[x-1] - dstp[x+u-1] )   +
                        abs( dstpnn[x]   - dstp[x+u] )     +
                        abs( dstpnn[x+1] - dstp[x+u+1] )   +
                        abs( p1 - p2 );
                    if( diff < min )
                    {
                        const int valt = ( p1 + p2 + 2 ) >> 2;
                        if( valt >= minm && valt <= maxm )
                        {
                            val = valt;
                            min = diff;
                            dir = u;
                        }
                    }
                }
                dstpn[x] = val;
                if( min == 7*nt ) dmskp[x] = neutral;
                else dmskp[x] = neutral + (dir << shift2);
            }
        }
        dstp += dst_pitch * 2;
        dstpn += dst_pitch * 2;
        dstpnn += dst_pitch * 2;
        dmskp += dmsk_pitch * 2;
        omskp += omsk_pitch * 2;
        omskn += omsk_pitch * 2;
    }
}

/**
 * Applies some extra filtering to smooth the edge direction mask
 * @param nmskp Pointer to the newly-filtered edge direction mask being read from
 * @param nmsk_pitch Stride of nmskp
 * @param omskp Pointer to the old unfiltered edge direction mask being read from
 * @param omsk_pitch Stride of osmkp
 * @param dstp Pointer to the output image being filtered in place
 * @param src_pitch Stride of dstp ....not sure why it's named this
 * @param field Field to filter
 * @param height Height of the full-frame output
 * @param width Width of dstp bitmap rows, as opposed to the pdded stride in src_pitch
 */
void FUNC(eedi2_post_process)(const pixel *nmskp, const int nmsk_pitch, const pixel *omskp, const int omsk_pitch,
                               pixel *dstp, const int src_pitch, const int field, const int height, const int width, const int depth, const pixel limlut[33])
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;
    const pixel shift2 = 2 + (depth - 8);

    nmskp += ( 2 - field ) * nmsk_pitch;
    omskp += ( 2 - field ) * omsk_pitch;
    dstp += ( 2 - field ) * src_pitch;
    pixel *srcpp = dstp - src_pitch;
    pixel *srcpn = dstp + src_pitch;

    for( int y = 2 - field; y < height - 1; y += 2 )
    {
        for (int x = 0; x < width; ++x )
        {
            const int lim = limlut[abs(nmskp[x]-neutral)>>shift2];
            if (abs(nmskp[x] - omskp[x]) > lim && omskp[x] != peak && omskp[x] != neutral)
            {
                dstp[x] = (srcpp[x] + srcpn[x] + 1) >> 1;
            }
        }
        nmskp += nmsk_pitch * 2;
        omskp += omsk_pitch * 2;
        srcpp += src_pitch * 2;
        dstp += src_pitch * 2;
        srcpn += src_pitch * 2;
    }
}

/**
 * Blurs the source field plane
 * @param src Pointer to the half-height source field plane
 * @param src_pitch Stride of src
 * @param tmp Pointer to a temporary buffer for juggling bitmaps
 * @param tmp_pitch Stride of tmp
 * @param dst Pointer to the destination to store the blurred field plane
 * @param dst_pitch Stride of dst
 * @param height Height of the half-height field-sized frame
 * @param width Width of dstp bitmap rows, as opposed to the padded stride in dst_pitch
 */
void FUNC(eedi2_gaussian_blur1)(const pixel * src, const int src_pitch, pixel *tmp, int tmp_pitch, pixel *dst, const int dst_pitch, const int height, const int width)
{
    const pixel *srcp = src;
    pixel *dstp = tmp;
    int x, y;

    for( y = 0; y < height; ++y )
    {
        dstp[0] = ( srcp[3] * 582 + srcp[2] * 7078 + srcp[1] * 31724 +
                    srcp[0] * 26152 + 32768 ) >> 16;
        dstp[1] = ( srcp[4] * 582 + srcp[3] * 7078 +
                    ( srcp[0] + srcp[2] ) * 15862 +
                    srcp[1] * 26152 + 32768 ) >> 16;
        dstp[2] = ( srcp[5] * 582 + ( srcp[0] + srcp[4] ) * 3539 +
                    ( srcp[1] + srcp[3] ) * 15862 +
                    srcp[2]*26152 + 32768 ) >> 16;
        for( x = 3; x < width - 3; ++x )
        {
            dstp[x] = ( ( srcp[x-3] + srcp[x+3] ) * 291 +
                        ( srcp[x-2] + srcp[x+2] ) * 3539 +
                        ( srcp[x-1] + srcp[x+1] ) * 15862 +
                        srcp[x] * 26152 + 32768 ) >> 16;
        }
        dstp[x] = ( srcp[x-3] * 582 + ( srcp[x-2] + srcp[x+2] ) * 3539 +
                    ( srcp[x-1] + srcp[x+1] ) * 15862 +
                    srcp[x]   * 26152 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x-3] * 582 + srcp[x-2] * 7078 +
                    ( srcp[x-1] + srcp[x+1] ) * 15862 +
                    srcp[x] * 26152 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x-3] * 582 + srcp[x-2] * 7078 +
                    srcp[x-1] * 31724 + srcp[x] * 26152 + 32768 ) >> 16;
        srcp += src_pitch;
        dstp += tmp_pitch;
    }
    srcp = tmp;
    dstp = dst;
    const pixel *src3p = srcp - tmp_pitch * 3;
    const pixel *src2p = srcp - tmp_pitch * 2;
    const pixel *srcpp = srcp - tmp_pitch;
    const pixel *srcpn = srcp + tmp_pitch;
    const pixel *src2n = srcp + tmp_pitch * 2;
    const pixel *src3n = srcp + tmp_pitch * 3;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src3n[x] * 582 + src2n[x] * 7078 + srcpn[x] * 31724 +
                     srcp[x] * 26152 + 32768 ) >> 16;
    }
    src3p += tmp_pitch;
    src2p += tmp_pitch;
    srcpp += tmp_pitch;
    srcp += tmp_pitch;
    srcpn += tmp_pitch;
    src2n += tmp_pitch;
    src3n += tmp_pitch;
    dstp += dst_pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src3n[x] * 582 + src2n[x] * 7078 +
                    ( srcpp[x] + srcpn[x] ) * 15862 +
                    srcp[x] * 26152 + 32768 ) >> 16;
    }
    src3p += tmp_pitch;
    src2p += tmp_pitch;
    srcpp += tmp_pitch;
    srcp += tmp_pitch;
    srcpn += tmp_pitch;
    src2n += tmp_pitch;
    src3n += tmp_pitch;
    dstp += dst_pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src3n[x] * 582 + ( src2p[x] + src2n[x] ) * 3539 +
                    ( srcpp[x] + srcpn[x] ) * 15862 +
                    srcp[x] * 26152 + 32768 ) >> 16;
    }
    src3p += src_pitch;
    src2p += src_pitch;
    srcpp += src_pitch;
    srcp += src_pitch;
    srcpn += src_pitch;
    src2n += src_pitch;
    src3n += src_pitch;
    dstp += dst_pitch;
    for( y = 3; y < height - 3; ++y )
    {
        for( x = 0; x < width; ++x )
        {
            dstp[x] = ( ( src3p[x] + src3n[x] ) * 291 +
                        ( src2p[x] + src2n[x] ) * 3539 +
                        ( srcpp[x] + srcpn[x] ) * 15862 +
                        srcp[x] * 26152 + 32768 ) >> 16;
        }
        src3p += tmp_pitch;
        src2p += tmp_pitch;
        srcpp += tmp_pitch;
        srcp += tmp_pitch;
        srcpn += tmp_pitch;
        src2n += tmp_pitch;
        src3n += tmp_pitch;
        dstp += dst_pitch;
    }
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src3p[x] * 582 + ( src2p[x] + src2n[x] ) *3539 +
                    ( srcpp[x] + srcpn[x] ) * 15862 +
                    srcp[x] * 26152 + 32768 ) >> 16;
    }
    src3p += tmp_pitch;
    src2p += tmp_pitch;
    srcpp += tmp_pitch;
    srcp += tmp_pitch;
    srcpn += tmp_pitch;
//    src2n += tmp_pitch;
//    src3n += tmp_pitch;
    dstp += dst_pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src3p[x] * 582 + src2p[x] * 7078 +
                    ( srcpp[x] + srcpn[x] ) * 15862 +
                     srcp[x] * 26152 + 32768 ) >> 16;
    }
    src3p += tmp_pitch;
    src2p += tmp_pitch;
    srcpp += tmp_pitch;
    srcp += tmp_pitch;
//    srcpn += tmp_pitch;
//    src2n += tmp_pitch;
//    src3n += tmp_pitch;
    dstp += dst_pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src3p[x] * 582   + src2p[x] * 7078 +
                    srcpp[x] * 31724 +  srcp[x] * 26152 + 32768 ) >> 16;
    }
}


/**
 * Blurs the spatial derivatives of the source field plane
 * @param src Pointer to the derivative array to filter
 * @param tmp Pointer to a temporary storage for the derivative array while it's being filtered
 * @param dst Pointer to the destination to store the filtered output derivative array
 * @param pitch Stride of the bitmap from which the src array is derived
 * @param height Height of the half-height field-sized frame from which the src array derivs were taken
 * @param width Width of the bitmap from which the src array is derived, as opposed to the padded stride in pitch
 */
void FUNC(eedi2_gaussian_blur_sqrt2)(const int *src, int *tmp, int *dst, const int pitch, const int height, const int width)
{
    const int *srcp = src;
    int * dstp = tmp;
    int x, y;

    for( y = 0; y < height; ++y )
    {
        x = 0;
        dstp[x] = ( srcp[x+4] * 678   + srcp[x+3] * 3902  + srcp[x+2] * 13618 +
                    srcp[x+1] * 28830 + srcp[x]   * 18508 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x+4] * 678   + srcp[x+3] * 3902 + srcp[x+2] * 13618 +
                    ( srcp[x-1] + srcp[x+1] ) *14415 +
                    srcp[x]   * 18508 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x+4] * 678   + srcp[x+3] * 3902 +
                    ( srcp[x-2] + srcp[x+2] ) * 6809 +
                    ( srcp[x-1] + srcp[x+1] ) * 14415 +
                    srcp[x]   * 18508 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x+4] * 678   + ( srcp[x-3] + srcp[x+3] ) * 1951 +
                    ( srcp[x-2] + srcp[x+2] ) * 6809 +
                    ( srcp[x-1] + srcp[x+1] ) * 14415 +
                    srcp[x]   * 18508 + 32768 ) >> 16;

        for( x = 4; x < width - 4; ++x )
        {
            dstp[x] = ( ( srcp[x-4] + srcp[x+4] ) * 339 +
                        ( srcp[x-3] + srcp[x+3] ) * 1951 +
                        ( srcp[x-2] + srcp[x+2] ) * 6809 +
                        ( srcp[x-1] + srcp[x+1] ) * 14415 +
                        srcp[x] * 18508 + 32768 ) >> 16;
        }

        dstp[x] = ( srcp[x-4] * 678 + ( srcp[x-3] + srcp[x+3] ) * 1951 +
                    ( srcp[x-2] + srcp[x+2] ) * 6809  +
                    ( srcp[x-1] + srcp[x+1] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x-4] * 678 + srcp[x-3] * 3902 +
                    ( srcp[x-2] + srcp[x+2] ) * 6809 +
                    ( srcp[x-1] + srcp[x+1] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x-4] * 678 + srcp[x+3] * 3902 + srcp[x-2] * 13618 +
                    ( srcp[x-1] + srcp[x+1] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 16;
        ++x;
        dstp[x] = ( srcp[x-4] * 678 + srcp[x-3] * 3902 + srcp[x-2] * 13618 +
                    srcp[x-1] * 28830 +
                    srcp[x] * 18508 + 32768 ) >> 16;
        srcp += pitch;
        dstp += pitch;
    }
    dstp = dst;
    srcp = tmp;
    const int * src4p = srcp - pitch * 4;
    const int * src3p = srcp - pitch * 3;
    const int * src2p = srcp - pitch * 2;
    const int * srcpp = srcp - pitch;
    const int * srcpn = srcp + pitch;
    const int * src2n = srcp + pitch * 2;
    const int * src3n = srcp + pitch * 3;
    const int * src4n = srcp + pitch * 4;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4n[x] * 678   + src3n[x] * 3902  +
                    src2n[x] * 13618 + srcpn[x] * 28830 +
                     srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
    srcpn += pitch;
    src2n += pitch;
    src3n += pitch;
    src4n += pitch;
    dstp += pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4n[x] * 678 + src3n[x] * 3902 + src2n[x] * 13618 +
                    ( srcpp[x] + srcpn[x] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
    srcpn += pitch;
    src2n += pitch;
    src3n += pitch;
    src4n += pitch;
    dstp += pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4n[x] * 678 + src3n[x] * 3902 +
                    ( src2p[x] + src2n[x] ) * 6809 +
                    ( srcpp[x] + srcpn[x] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
    srcpn += pitch;
    src2n += pitch;
    src3n += pitch;
    src4n += pitch;
    dstp += pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4n[x] * 678 + ( src3p[x] + src3n[x] ) * 1951 +
                    ( src2p[x] + src2n[x] ) * 6809 +
                    ( srcpp[x] + srcpn[x] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
    srcpn += pitch;
    src2n += pitch;
    src3n += pitch;
    src4n += pitch;
    dstp += pitch;
    for( y = 4; y < height - 4; ++y )
    {
        for( x = 0; x < width; ++x )
        {
            dstp[x] = ( ( src4p[x] + src4n[x] ) * 339 +
                        ( src3p[x] + src3n[x] ) * 1951 +
                        ( src2p[x] + src2n[x] ) * 6809 +
                        ( srcpp[x] + srcpn[x] ) * 14415 +
                        srcp[x] * 18508 + 32768 ) >> 18;
        }
        src4p += pitch;
        src3p += pitch;
        src2p += pitch;
        srcpp += pitch;
        srcp += pitch;
        srcpn += pitch;
        src2n += pitch;
        src3n += pitch;
        src4n += pitch;
        dstp += pitch;
    }
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4p[x] * 678 +
                    ( src3p[x] + src3n[x] ) * 1951 +
                    ( src2p[x] + src2n[x] ) * 6809 +
                    ( srcpp[x] + srcpn[x] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
    srcpn += pitch;
    src2n += pitch;
//    src3n += pitch;
//    src4n += pitch;
    dstp += pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4p[x] * 678 + src3p[x] * 3902 +
                    ( src2p[x] + src2n[x] ) * 6809 +
                    ( srcpp[x] + srcpn[x] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
    srcpn += pitch;
//    src2n += pitch;
//    src3n += pitch;
//    src4n += pitch;
    dstp += pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4p[x] * 678 + src3p[x] * 3902 + src2p[x] * 13618 +
                    ( srcpp[x] + srcpn[x] ) * 14415 +
                    srcp[x] * 18508 + 32768 ) >> 18;
    }
    src4p += pitch;
    src3p += pitch;
    src2p += pitch;
    srcpp += pitch;
    srcp += pitch;
//    srcpn += pitch;
//    src2n += pitch;
//    src3n += pitch;
//    src4n += pitch;
    dstp += pitch;
    for( x = 0; x < width; ++x )
    {
        dstp[x] = ( src4p[x] * 678   + src3p[x] * 3902 +
                    src2p[x] * 13618 + srcpp[x] * 28830 +
                    srcp[x]  * 18508 + 32768 ) >> 18;
    }
}

/**
 * Finds spatial derivatives for a a source field plane
 * @param srcp Pointer to the plane to derive
 * @param src_pitch Stride of srcp
 * @param height Height of the half-height field-sized frame
 * @param width Width of srcp bitmap rows, as opposed to the padded stride in src_pitch
 * @param x2 Pointed to the array to store the x/x derivatives
 * @param y2 Pointer to the array to store the y/y derivatives
 * @param xy Pointer to the array to store the x/y derivatives
 */
void FUNC(eedi2_calc_derivatives)(const pixel *srcp, const int src_pitch, const int height, const int width, int *x2, int *y2, int *xy, const int depth)
{
    const pixel shift = depth - 8;

    const pixel *srcpp = srcp - src_pitch;
    const pixel *srcpn = srcp + src_pitch;
    int x, y;
    {
        const int Ix = (srcp[1] -  srcp[0]) >> shift;
        const int Iy = (srcp[0] - srcpn[0]) >> shift;
        x2[0] = ( Ix * Ix ) >> 1;
        y2[0] = ( Iy * Iy ) >> 1;
        xy[0] = ( Ix * Iy ) >> 1;
    }
    for( x = 1; x < width - 1; ++x )
    {
        const int Ix = (srcp[x+1] -  srcp[x-1]) >> shift;
        const int Iy = (srcp[x]   - srcpn[x]) >> shift;
        x2[x] = ( Ix * Ix ) >> 1;
        y2[x] = ( Iy * Iy ) >> 1;
        xy[x] = ( Ix * Iy ) >> 1;
    }
    {
        const int Ix = (srcp[x] -  srcp[x-1]) >> shift;
        const int Iy = (srcp[x] - srcpn[x]) >> shift;
        x2[x] = ( Ix * Ix ) >> 1;
        y2[x] = ( Iy * Iy ) >> 1;
        xy[x] = ( Ix * Iy ) >> 1;
    }
    srcpp += src_pitch;
    srcp += src_pitch;
    srcpn += src_pitch;
    x2 += src_pitch;
    y2 += src_pitch;
    xy += src_pitch;
    for( y = 1; y < height - 1; ++y )
    {
        {
            const int Ix =  (srcp[1] -  srcp[0]) >> shift;
            const int Iy = (srcpp[0] - srcpn[0]) >> shift;
            x2[0] = ( Ix * Ix ) >> 1;
            y2[0] = ( Iy * Iy ) >> 1;
            xy[0] = ( Ix * Iy ) >> 1;
        }
        for ( x = 1; x < width - 1; ++x )
        {
            const int Ix =  (srcp[x+1] -  srcp[x-1]) >> shift;
            const int Iy = (srcpp[x]   - srcpn[x]) >> shift;
            x2[x] = ( Ix * Ix ) >> 1;
            y2[x] = ( Iy * Iy ) >> 1;
            xy[x] = ( Ix * Iy ) >> 1;
        }
        {
            const int Ix =  (srcp[x] -  srcp[x-1]) >> shift;
            const int Iy = (srcpp[x] - srcpn[x]) >> shift;
            x2[x] = ( Ix *Ix ) >> 1;
            y2[x] = ( Iy *Iy ) >> 1;
            xy[x] = ( Ix *Iy ) >> 1;
        }
        srcpp += src_pitch;
        srcp += src_pitch;
        srcpn += src_pitch;
        x2 += src_pitch;
        y2 += src_pitch;
        xy += src_pitch;
    }
    {
        const int Ix =  (srcp[1] - srcp[0]) >> shift;
        const int Iy = (srcpp[0] - srcp[0]) >> shift;
        x2[0] = ( Ix * Ix ) >> 1;
        y2[0] = ( Iy * Iy ) >> 1;
        xy[0] = ( Ix * Iy ) >> 1;
    }
    for( x = 1; x < width - 1; ++x )
    {
        const int Ix =  (srcp[x+1] - srcp[x-1]) >> shift;
        const int Iy = (srcpp[x]   - srcp[x]) >> shift;
        x2[x] = ( Ix * Ix ) >> 1;
        y2[x] = ( Iy * Iy ) >> 1;
        xy[x] = ( Ix * Iy ) >> 1;
    }
    {
        const int Ix =  (srcp[x] - srcp[x-1]) >> shift;
        const int Iy = (srcpp[x] - srcp[x]) >> shift;
        x2[x] = ( Ix * Ix ) >> 1;
        y2[x] = ( Iy * Iy ) >> 1;
        xy[x] = ( Ix * Iy ) >> 1;
    }
}

/**
 * Filters junctions and corners for the output image
 * @param x2 Pointer to the x/x derivatives
 * @param y2 Pointer to the y/y derivatives
 * @param xy Pointer to the x/y derivatives
 * @param pitch Stride of the source field plane from which the derivatives were calculated
 * @param mskp Pointer to the edge direction mask
 * @param msk_pitch Stride of mskp
 * @param dstp Pointer to the output image being filtered in place
 * @param dst_pitch Stride of dstp
 * @param height Height of the full-frame output plane
 * @param width Width of dstp bitmap rows, as opposed to the padded stride in dst_pitch
 * @param field Field to filter
 */
void FUNC(eedi2_post_process_corner)(int *x2, int *y2, int *xy, const int pitch, const pixel *mskp, const int msk_pitch,
                                     pixel *dstp, const int dst_pitch, const int height, const int width, const int field, const int depth)
{
    const pixel neutral = 1 << (depth - 1);
    const pixel peak = (1 << depth) - 1;

    mskp += ( 8 - field ) * msk_pitch;
    dstp += ( 8 - field ) * dst_pitch;
    pixel * dstpp = dstp - dst_pitch;
    pixel * dstpn = dstp + dst_pitch;
    x2 += pitch * 3;
    y2 += pitch * 3;
    xy += pitch * 3;
    int *x2n = x2 + pitch;
    int *y2n = y2 + pitch;
    int *xyn = xy + pitch;

    for (int y = 8 - field; y < height - 7; y += 2)
    {
        for (int x = 4; x < width - 4; ++x)
        {
            if( mskp[x] == peak || mskp[x] == neutral ) continue;
            const int c1 = (int)( x2[x]  *  y2[x] -  xy[x] * xy[x] - 0.09 *
                                  ( x2[x]  + y2[x] )  * ( x2[x]  + y2[x] ) );
            const int c2 = (int)( x2n[x] * y2n[x] - xyn[x]* xyn[x] - 0.09 *
                                  ( x2n[x] + y2n[x] ) * ( x2n[x] + y2n[x] ) );
            if (c1 > 775 || c2 > 775)
                dstp[x] = ( dstpp[x] + dstpn[x] + 1 ) >> 1;
        }
        mskp += msk_pitch * 2;
        dstpp += dst_pitch * 2;
        dstp += dst_pitch * 2;
        dstpn += dst_pitch * 2;
        x2 += pitch;
        x2n += pitch;
        y2 += pitch;
        y2n += pitch;
        xy += pitch;
        xyn += pitch;
    }
}

#undef pixel
#undef FUNC
#undef BPS
