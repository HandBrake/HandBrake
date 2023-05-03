/* decomb.h

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_DECOMB_H
#define HANDBRAKE_DECOMB_H

#define MODE_DECOMB_YADIF       1 // Use yadif
#define MODE_DECOMB_BLEND       2 // Use blending interpolation
#define MODE_DECOMB_CUBIC       4 // Use cubic interpolation
#define MODE_DECOMB_EEDI2       8 // Use EEDI2 interpolation
#define MODE_DECOMB_BOB        16 // Deinterlace each field to a separate frame
#define MODE_DECOMB_SELECTIVE  32 // Selectively deinterlace based on comb detection

#define MODE_YADIF_ENABLE       1
#define MODE_YADIF_SPATIAL      2
#define MODE_XXDIF_BOB          4

#endif // HANDBRAKE_DECOMB_H
