/* $Id: TargetSizeField.h,v 1.2 2003/10/09 23:33:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "Common.h"

@interface HBTargetSizeField : NSTextField

{
    HBTitle                * fTitle;
    IBOutlet NSTextField   * fBitrateField;
    IBOutlet NSPopUpButton * fSecondaryLanguagePopUp;
    IBOutlet NSPopUpButton * fAudioBitratePopUp;
}

- (void) SetHBTitle: (HBTitle *) title;
- (void) UpdateBitrate;

@end
