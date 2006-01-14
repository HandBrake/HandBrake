/* $Id: TargetSizeField.h,v 1.3 2004/01/28 14:41:31 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "HandBrake.h"

@interface HBTargetSizeField : NSTextField

{
    HBTitle                * fTitle;
    IBOutlet NSPopUpButton * fRipFormatPopUp;
    IBOutlet NSTextField   * fRipCustomField;
    IBOutlet NSPopUpButton * fRipLang2PopUp;
    IBOutlet NSPopUpButton * fRipAudBitPopUp;
}

- (void) SetHBTitle: (HBTitle *) title;
- (void) UpdateBitrate;

@end
