/* $Id: downmix.h,v 1.51 2005/11/04 13:09:40 stebbins Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef DOWNMIX_H
#define DOWNMIX_H

typedef float hb_sample_t;

typedef struct
{
    int chan_map[10][2][8];
    int inv_chan_map[10][2][8];
} hb_chan_map_t;

typedef struct
{
    int            mode_in;
    int            mode_out;
    int            nchans_in;
    int            nchans_out;
    hb_sample_t    matrix[8][8];
    int            matrix_initialized;
    hb_sample_t    clev;
    hb_sample_t    slev;
    hb_sample_t    level;
    hb_sample_t    bias;
    hb_chan_map_t  map_in;
    hb_chan_map_t  map_out;

    int center;
    int left_surround;
    int right_surround;
    int rear_left_surround;
    int rear_right_surround;
} hb_downmix_t;

// For convenience, a map to convert smpte channel layout
// to QuickTime channel layout.
// Map Indicies are mode, lfe, channel respectively
extern hb_chan_map_t hb_smpte_chan_map;
extern hb_chan_map_t hb_ac3_chan_map;
extern hb_chan_map_t hb_qt_chan_map;

hb_downmix_t * hb_downmix_init(int layout, int mixdown);
void hb_downmix_close( hb_downmix_t **downmix );
int hb_downmix_set_mode( hb_downmix_t * downmix, int layout, int mixdown );
void hb_downmix_set_level( hb_downmix_t * downmix, hb_sample_t clev, hb_sample_t slev, hb_sample_t level );
void hb_downmix_adjust_level( hb_downmix_t * downmix );
void hb_downmix_set_bias( hb_downmix_t * downmix, hb_sample_t bias );
void hb_downmix_set_chan_map( 
    hb_downmix_t * downmix, 
    hb_chan_map_t * map_in, 
    hb_chan_map_t * map_out );
void hb_downmix( hb_downmix_t * downmix, hb_sample_t * dst, hb_sample_t * src, int nsamples);
void hb_layout_remap( 
    hb_chan_map_t * map_in, 
    hb_chan_map_t * map_out, 
    int layout, 
    hb_sample_t * samples, 
    int nsamples );
int hb_need_downmix( int layout, int mixdown );

#endif /* DOWNMIX_H */
