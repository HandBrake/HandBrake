/*  NSWindow+HBAdditions.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSWindow+HBAdditions.h"

@implementation NSWindow (HBAdditions)

- (void)HB_resizeToBestSizeForViewSize:(NSSize)viewSize keepInScreenRect:(BOOL)keepInScreenRect centerPoint:(NSPoint)center animate:(BOOL)animateFlag
{
    NSSize currentSize = self.contentView.frame.size;
    NSRect frame = self.frame;

    // Calculate border around content region of the frame
    int borderX = (int)(frame.size.width - currentSize.width);
    int borderY = (int)(frame.size.height - currentSize.height);

    // Make sure the frame is smaller than the screen
    NSSize maxSize = self.screen.visibleFrame.size;

    // if we are not Scale To Screen, put an 10% of visible screen on the window
    maxSize.width = maxSize.width * 0.90;
    maxSize.height = maxSize.height * 0.90;

    // Set the new frame size
    // Add the border to the new frame size so that the content region
    // of the frame is large enough to accommodate the preview image
    frame.size.width = viewSize.width + borderX;
    frame.size.height = viewSize.height + borderY;

    // compare frame to max size of screen
    if (frame.size.width > maxSize.width)
    {
        frame.size.width = maxSize.width;
    }
    if (frame.size.height > maxSize.height)
    {
        frame.size.height = maxSize.height;
    }

    // Since upon launch we can open up the preview window if it was open
    // the last time we quit (and at the size it was) we want to make
    // sure that upon resize we do not have the window off the screen
    // So check the origin against the screen origin and adjust if
    // necessary.

    if (center.x == 0 && center.y == 0)
    {
        center = [self HB_centerPoint];
    }
    frame.origin.x = center.x - floor(frame.size.width / 2);
    frame.origin.y = center.y - floor(frame.size.height / 2);

    if (keepInScreenRect)
    {
        NSSize screenSize = self.screen.visibleFrame.size;
        NSPoint screenOrigin = self.screen.visibleFrame.origin;

        // our origin is off the screen to the left
        if (frame.origin.x < screenOrigin.x)
        {
            // so shift our origin to the right
            frame.origin.x = screenOrigin.x;
        }
        else if ((frame.origin.x + frame.size.width) > (screenOrigin.x + screenSize.width))
        {
            // the right side of the preview is off the screen, so shift to the left
            frame.origin.x = (screenOrigin.x + screenSize.width) - frame.size.width;
        }

        // our origin is off the screen to the bottom
        if (frame.origin.y < screenOrigin.y)
        {
            // so shift our origin to the top
            frame.origin.y = screenOrigin.y;
        }
        else if ((frame.origin.y + frame.size.height) > (screenOrigin.y + screenSize.height))
        {
            // the top side of the preview is off the screen, so shift to the bottom
            frame.origin.y = (screenOrigin.y + screenSize.height) - frame.size.height;
        }
    }

    [self setFrame:frame display:YES animate:animateFlag];
}

- (NSPoint)HB_centerPoint
{
    NSPoint center = NSMakePoint(floor(self.frame.origin.x + self.frame.size.width / 2),
                                 floor(self.frame.origin.y + self.frame.size.height / 2));
    return center;
}

- (BOOL)HB_endEditing
{
    BOOL success;
    NSRange selectedRange = NSMakeRange(0, 0);
    id responder = self.firstResponder;

    // If we're dealing with the field editor, the real first responder is
    // its delegate.
    if ((responder != nil) && [responder isKindOfClass:[NSTextView class]] && [(NSTextView *)responder isFieldEditor])
    {
        responder = ([[responder delegate] isKindOfClass:[NSResponder class]]) ? [responder delegate] : nil;
        if ([responder isKindOfClass:[NSTextField class]])
        {
            NSTextField *textField = (NSTextField *)responder;
            selectedRange = textField.currentEditor.selectedRange;
        }
    }

    success = [self makeFirstResponder:nil];

    // Return first responder status.
    if (success && responder != nil)
    {
        [self makeFirstResponder:responder];
        if ([responder isKindOfClass:[NSTextField class]])
        {
            NSTextField *textField = (NSTextField *)responder;
            textField.currentEditor.selectedRange = selectedRange;
        }
    }

    return success;
}

- (void)HB_forceEndEditing
{
    [self endEditingFor:nil];
}



@end
