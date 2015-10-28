/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPictureController.h"
#import "HBFilters.h"
#import "HBPicture.h"

static void *HBPictureControllerContext = &HBPictureControllerContext;

@interface HBPictureController ()
{
    /* Picture Sizing */
    IBOutlet NSBox           * fPictureSizeBox;
    IBOutlet NSBox           * fPictureCropBox;

	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSStepper       * fWidthStepper;
    IBOutlet NSStepper       * fHeightStepper;

}

@end

@implementation HBPictureController

- (instancetype)init
{
	if (self = [super initWithWindowNibName:@"PictureSettings"])
	{
        // NSWindowController likes to lazily load its window. However since
        // this controller tries to set all sorts of outlets before the window
        // is displayed, we need it to load immediately. The correct way to do
        // this, according to the documentation, is simply to invoke the window
        // getter once.
        //
        // If/when we switch a lot of this stuff to bindings, this can probably
        // go away.
        [self window];

        [self addObserver:self forKeyPath:@"self.picture.anamorphicMode" options:NSKeyValueObservingOptionInitial context:HBPictureControllerContext];
        [self addObserver:self forKeyPath:@"self.picture.modulus" options:NSKeyValueObservingOptionInitial context:HBPictureControllerContext];
    }

	return self;
}

- (void)dealloc
{
    NSArray *observerdKeyPaths = @[@"self.picture.anamorphicMode", @"self.picture.modulus"];
    @try
    {
        for (NSString *keyPath in observerdKeyPaths)
        {
            [self removeObserver:self forKeyPath:keyPath context:HBPictureControllerContext];
        }

    } @catch (NSException * __unused exception) {}
}

- (void)windowDidLoad
{
    [[self window] setExcludedFromWindowsMenu:YES];

    /* Populate the Anamorphic NSPopUp button here */
    [fAnamorphicPopUp removeAllItems];
    [fAnamorphicPopUp addItemsWithTitles:@[@"None", @"Strict", @"Loose", @"Custom"]];

    [self resizeInspectorForTab:nil];
    [self adjustSizingDisplay:nil];
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBPictureControllerContext)
    {
        // We use KVO to update the panel
        // and notify the main controller of the changes
        // in the filters and picture settings.

        if ([keyPath isEqualToString:@"self.picture.anamorphicMode"])
        {
            [self adjustSizingDisplay:nil];
        }
        else if ([keyPath isEqualToString:@"self.picture.modulus"])
        {
            [fWidthStepper setIncrement:self.picture.modulus];
            [fHeightStepper setIncrement:self.picture.modulus];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark -
#pragma mark Interface Resize

/**
 * This method is used to detect clicking on a tab in fSizeFilterView
 */
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    [self resizeInspectorForTab:nil];
}

/**
 * resizeInspectorForTab is called at launch, and each time either the
 * Size or Filters tab is clicked. Size gives a horizontally oriented
 * inspector and Filters is a vertically aligned inspector.
 */
- (void)resizeInspectorForTab:(id)sender
{
    NSRect frame = self.window.frame;
    NSSize screenSize = self.window.screen.frame.size;
    NSPoint screenOrigin = self.window.screen.frame.origin;

    frame.size.width = 30.0 + fPictureSizeBox.frame.size.width + fPictureCropBox.frame.size.width;

    /* get delta's for the change in window size */
    CGFloat deltaX = frame.size.width - [[self window] frame].size.width;
    CGFloat deltaY = frame.size.height - [[self window] frame].size.height;

    /* change the inspector origin via the deltaY */
    frame.origin.y -= deltaY;
    /* keep the inspector centered so the tabs stay in place */
    frame.origin.x -= deltaX / 2.0;

    /* we make sure we are not horizontally off of our screen.
     * this would be the case if we are on the vertical filter tab
     * and we hit the size tab and the inspector grows horizontally
     * off the screen to the right
     */
    if ((frame.origin.x + frame.size.width) > (screenOrigin.x + screenSize.width))
    {
        /* the right side of the preview is off the screen, so shift to the left */
        frame.origin.x = (screenOrigin.x + screenSize.width) - frame.size.width;
    }

    [[self window] setFrame:frame display:YES animate:YES];
}

- (void)adjustSizingDisplay:(id)sender
{
    NSSize pictureSizingBoxSize = [fPictureSizeBox frame].size;

    NSPoint fPictureSizeBoxOrigin = [fPictureSizeBox frame].origin;
    NSSize pictureCropBoxSize = [fPictureCropBox frame].size;
    NSPoint fPictureCropBoxOrigin = [fPictureCropBox frame].origin;

    if ([fAnamorphicPopUp indexOfSelectedItem] == 3)
    {   // custom / power user jamboree
        pictureSizingBoxSize.width = 350;
    }
    else
    {
        pictureSizingBoxSize.width = 200;
    }

    /* Check to see if we have changed the size from current */
    if (pictureSizingBoxSize.height != [fPictureSizeBox frame].size.height ||
        pictureSizingBoxSize.width != [fPictureSizeBox frame].size.width)
    {
        /* Get our delta for the change in picture size box height */
        CGFloat deltaYSizeBoxShift = pictureSizingBoxSize.height -
                                     [fPictureSizeBox frame].size.height;
        fPictureSizeBoxOrigin.y -= deltaYSizeBoxShift;
        /* Get our delta for the change in picture size box width */
        CGFloat deltaXSizeBoxShift = pictureSizingBoxSize.width -
                                     [fPictureSizeBox frame].size.width;
        //fPictureSizeBoxOrigin.x += deltaXSizeBoxShift;
        /* set our new Picture size box size */
        [fPictureSizeBox setFrameSize:pictureSizingBoxSize];
        [fPictureSizeBox setFrameOrigin:fPictureSizeBoxOrigin];

        pictureCropBoxSize.height += deltaYSizeBoxShift;
        fPictureCropBoxOrigin.y -= deltaYSizeBoxShift;
        fPictureCropBoxOrigin.x += deltaXSizeBoxShift;

        [fPictureCropBox setFrameSize:pictureCropBoxSize];
        [[fPictureCropBox animator] setFrameOrigin:fPictureCropBoxOrigin];
    }

    /* now we call to resize the entire inspector window */
    [self resizeInspectorForTab:nil];
}

#pragma mark -

/**
 * Displays and brings the picture window to the front
 */
- (void)showWindow:(id)sender
{
    if (self.window.isVisible)
    {
        [self.window close];
    }
    else
    {
        [super showWindow:self];
    }

    [self resizeInspectorForTab:nil];
    [self adjustSizingDisplay:nil];
}

@end
