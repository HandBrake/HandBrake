/* $Id: TargetSizeField.h,v 1.1.1.1 2003/11/03 12:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "HandBrake.h"

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
