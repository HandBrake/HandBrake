/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "Controller.h"
#import "PictureController.h"
#import "HBPreviewController.h"

#import "HBTitle.h"

@interface HBCustomFilterTransformer : NSValueTransformer
@end

@implementation HBCustomFilterTransformer

+ (Class)transformedValueClass
{
    return [NSNumber class];
}

- (id)transformedValue:(id)value
{
    if ([value intValue] == 1)
        return @NO;
    else
        return @YES;
}

+ (BOOL)allowsReverseTransformation
{
    return NO;
}

@end

static void *HBPictureControllerContext = &HBPictureControllerContext;

@interface HBPictureController ()
{
    /* Picture Sizing */
    IBOutlet NSTabView       * fSizeFilterView;
    IBOutlet NSBox           * fPictureSizeBox;
    IBOutlet NSBox           * fPictureCropBox;

	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSStepper       * fWidthStepper;
    IBOutlet NSStepper       * fHeightStepper;

    /* Video Filters */
    IBOutlet NSBox           * fDetelecineBox;
    IBOutlet NSBox           * fDecombDeinterlaceBox;
    IBOutlet NSBox           * fDecombBox;
    IBOutlet NSBox           * fDeinterlaceBox;

    IBOutlet NSTextField     * fDeblockField;

    IBOutlet NSTextField    *fDenoisePreset;
    IBOutlet NSPopUpButton  *fDenoisePresetPopUp;
    IBOutlet NSTextField    *fDenoiseTuneLabel;
    IBOutlet NSPopUpButton  *fDenoiseTunePopUp;
    IBOutlet NSTextField    *fDenoiseCustomLabel;
    IBOutlet NSTextField    *fDenoiseCustomField;
}

@end

@implementation HBPictureController

- (instancetype)init
{
	if (self = [super initWithWindowNibName:@"PictureSettings"])
	{
        _filters = [[HBFilters alloc] init];
        _picture = [[HBPicture alloc] init];

        // NSWindowController likes to lazily load its window. However since
        // this controller tries to set all sorts of outlets before the window
        // is displayed, we need it to load immediately. The correct way to do
        // this, according to the documentation, is simply to invoke the window
        // getter once.
        //
        // If/when we switch a lot of this stuff to bindings, this can probably
        // go away.
        [self window];

        // Add the observers for the filters values
        NSArray *observerdKeyPaths = @[@"self.filters.useDecomb", @"self.filters.deblock",
                                       @"self.filters.denoise", @"self.filters.denoisePreset"];
        for (NSString *keyPath in observerdKeyPaths)
        {
            [self addObserver:self forKeyPath:keyPath options:NSKeyValueObservingOptionInitial context:HBPictureControllerContext];
        }

        [self addObserver:self forKeyPath:@"self.picture.anamorphicMode" options:NSKeyValueObservingOptionInitial context:HBPictureControllerContext];
        [self addObserver:self forKeyPath:@"self.picture.modulus" options:NSKeyValueObservingOptionInitial context:HBPictureControllerContext];
    }

	return self;
}

- (void) dealloc
{
    NSArray *observerdKeyPaths = @[@"self.filters.useDecomb", @"self.filters.deblock",
                                   @"self.filters.denoise", @"self.filters.denoisePreset"];
    @try {
        for (NSString *keyPath in observerdKeyPaths)
        {
            [self removeObserver:self forKeyPath:keyPath];
        }

    } @catch (NSException * __unused exception) {}

    self.filters = nil;
    self.picture = nil;

    [super dealloc];
}

- (void)windowDidLoad
{
    [[self window] setExcludedFromWindowsMenu:YES];

    // Set the panel appearance explicity to aqua.
    // can be removed when Apple will fix UI appearance on Yosemite.
    if (NSClassFromString(@"NSVisualEffectView")) {
        [self.window setAppearance:[NSClassFromString(@"NSAppearance") appearanceNamed:@"NSAppearanceNameAqua"]];
    }

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
        else if ([keyPath isEqualToString:@"self.filters.useDecomb"])
        {
            if (self.filters.useDecomb)
            {
                [fDecombBox setHidden:NO];
                [fDeinterlaceBox setHidden:YES];
            }
            else
            {
                [fDecombBox setHidden:YES];
                [fDeinterlaceBox setHidden:NO];
            }
        }
        else if ([keyPath isEqualToString:@"self.filters.deblock"])
        {
            // The minimum deblock value is 5,
            // set it to 0 if the value is
            // less than 4.
            if (self.filters.deblock == 4)
            {
                [fDeblockField setStringValue: @"Off"];
                self.filters.deblock = 0;
            }
            else if (self.filters.deblock > 4)
            {
                [fDeblockField setStringValue:[NSString stringWithFormat: @"%.0ld", (long)self.filters.deblock]];
            }
        }
        else if ([keyPath isEqualToString:@"self.filters.denoise"] || [keyPath isEqualToString:@"self.filters.denoisePreset"])
        {
            [self validateDenoiseUI];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

/**
 *  Validates the denoise UI items,
 *  disables/enables the right ones.
 */
- (void)validateDenoiseUI
{
    if ([self.filters.denoise isEqualToString:@"off"])
    {
        NSArray *uiElements = @[fDenoisePreset, fDenoisePresetPopUp,
                                fDenoiseTuneLabel, fDenoiseTunePopUp,
                                fDenoiseCustomLabel, fDenoiseCustomField];
        for (NSView *view in uiElements)
            [view setHidden:YES];
    }
    else
    {
        NSArray *uiElements = @[fDenoisePreset, fDenoisePresetPopUp];
        for (NSView *view in uiElements)
            [view setHidden:NO];

        if ([self.filters.denoisePreset isEqualToString:@"none"])
        {
            [fDenoiseTuneLabel setHidden:YES];
            [fDenoiseTunePopUp setHidden:YES];
            [fDenoiseCustomLabel setHidden:NO];
            [fDenoiseCustomField setHidden:NO];
        }
        else if ([self.filters.denoise isEqualToString:@"hqdn3d"])
        {
            [fDenoiseTuneLabel setHidden:YES];
            [fDenoiseTunePopUp setHidden:YES];
            [fDenoiseCustomLabel setHidden:YES];
            [fDenoiseCustomField setHidden:YES];
        }
        else
        {
            [fDenoiseTuneLabel setHidden:NO];
            [fDenoiseTunePopUp setHidden:NO];
            [fDenoiseCustomLabel setHidden:YES];
            [fDenoiseCustomField setHidden:YES];
        }
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
- (void) resizeInspectorForTab: (id) sender
{
    NSRect frame = [[self window] frame];
    NSSize screenSize = [[[self window] screen] frame].size;
    NSPoint screenOrigin = [[[self window] screen] frame].origin;

    /* We base our inspector size/layout on which tab is active for fSizeFilterView */
    /* we are 1 which is Filters*/
    if ([fSizeFilterView indexOfTabViewItem: [fSizeFilterView selectedTabViewItem]] == 1)
    {
        frame.size.width = 484;
        /* we glean the height from the size of the boxes plus the extra window space
         * needed for non boxed display
         */
        frame.size.height = 100.0 + [fDetelecineBox frame].size.height + [fDecombDeinterlaceBox frame].size.height;
        /* Hide the size readout at the bottom as the vertical inspector is not wide enough */
    }
    else // we are Tab index 0 which is size
    {
        frame.size.width = 30.0 + [fPictureSizeBox frame].size.width + [fPictureCropBox frame].size.width;
        frame.size.height = [fPictureSizeBox frame].size.height + 90;
        /* hide the size summary field at the bottom */
    }
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

- (void) adjustSizingDisplay: (id) sender
{
    NSSize pictureSizingBoxSize = [fPictureSizeBox frame].size;

    NSPoint fPictureSizeBoxOrigin = [fPictureSizeBox frame].origin;
    NSSize pictureCropBoxSize = [fPictureCropBox frame].size;
    NSPoint fPictureCropBoxOrigin = [fPictureCropBox frame].origin;

    if ([fAnamorphicPopUp indexOfSelectedItem] == HB_ANAMORPHIC_CUSTOM)
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
- (void)showPictureWindow
{
    if ([[self window] isVisible])
    {
        [[self window] close];
    }
    else
    {
        [self showWindow:self];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PictureSizeWindowIsOpen"];
    }

    [self resizeInspectorForTab:nil];
    [self adjustSizingDisplay:nil];
}

- (IBAction) showPreviewWindow: (id) sender
{
    [self.delegate showPreviewWindow:sender];
}

- (void) windowWillClose: (NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PictureSizeWindowIsOpen"];
}

@end
