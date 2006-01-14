/* $Id: HandBrake.h,v 1.5 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_HANDBRAKE_H
#define HB_HANDBRAKE_H

#include <Application.h>

class HBWindow;

class HBApp : public BApplication
{
    public:
                   HBApp( bool debug );

        HBWindow * fWindow;
};

#endif
