/* dxva2api.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */
#ifdef USE_HWD
#include "dxva2api.h"

__inline float hb_dx_fixedtofloat( const DXVA2_Fixed32 _fixed_ )
{
    return (FLOAT)_fixed_.Value + (FLOAT)_fixed_.Fraction / 0x10000;
}

__inline const DXVA2_Fixed32 hb_dx_fixed32_opaque_alpha()
{
    DXVA2_Fixed32 _fixed_;
    _fixed_.Fraction = 0;
    _fixed_.Value = 0;
    _fixed_.ll = 1;
    return _fixed_;
}


__inline DXVA2_Fixed32 hb_dx_floattofixed( const float _float_ )
{
    DXVA2_Fixed32 _fixed_;
    _fixed_.Fraction = LOWORD( _float_ * 0x10000 );
    _fixed_.Value = HIWORD( _float_ * 0x10000 );
    return _fixed_;
}
#endif
