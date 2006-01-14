/* $Id: TargetSizeField.mm,v 1.4 2004/01/28 14:41:31 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "TargetSizeField.h"

@implementation HBTargetSizeField

- (void) textDidBeginEditing: (NSNotification *) notification
{
    [self UpdateBitrate];
    [super textDidBeginEditing: notification];
}

- (void) textDidEndEditing: (NSNotification *) notification
{
    [self UpdateBitrate];
    [super textDidEndEditing: notification];
}

- (void) textDidChange: (NSNotification *) notification
{
    [self UpdateBitrate];
    [super textDidChange: notification];
}

- (void) SetHBTitle: (HBTitle*) title
{
    fTitle = title;
}

- (void) UpdateBitrate
{
    int size   = [self intValue];
    int format = [fRipFormatPopUp indexOfSelectedItem];
    int muxer  = ( format == 0 ) ? HB_MUX_MP4 : ( ( format == 1 ) ?
            HB_MUX_OGM : HB_MUX_AVI );
    int audioCount = ( [fRipLang2PopUp selectedItem] ==
            [fRipLang2PopUp lastItem] ) ? 1 : 2;
    int audioBitrate = [[fRipAudBitPopUp titleOfSelectedItem] intValue];

    [fRipCustomField setIntValue: HBGetBitrateForSize( fTitle, size,
            muxer, audioCount, audioBitrate )];
}

@end
