/* $Id: TargetSizeField.mm,v 1.4 2003/10/09 23:33:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
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
    int64_t available;
    available  = (int64_t) [self intValue] * 1024 * 1024;

    /* AVI headers */
    available -= 2048;

    /* Video chunk headers (8 bytes / frame)
       and index (16 bytes / frame) */
    available -= 24 * fTitle->fLength * fTitle->fRate / fTitle->fScale;

    /* Audio tracks */
    available -= ( ( [[fSecondaryLanguagePopUp titleOfSelectedItem]
                       compare: @"None"] == NSOrderedSame ) ? 1 : 2 ) *
                 ( fTitle->fLength *
                   [[fAudioBitratePopUp titleOfSelectedItem] intValue] *
                   128 + 24 * fTitle->fLength * 44100 / 1152 );
    
    if( available < 0 )
    {
        [fBitrateField setIntValue: 0];
    }
    else
    {
        [fBitrateField setIntValue:
            available / ( 128 * fTitle->fLength )];
    }
}

@end
