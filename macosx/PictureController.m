/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "Controller.h"
#import "PictureController.h"
#import "HBPreviewController.h"

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
    hb_title_t               * fTitle;

    HBPreviewController        * fPreviewController;

    /* Picture Sizing */
    IBOutlet NSTabView       * fSizeFilterView;
    IBOutlet NSBox           * fPictureSizeBox;
    IBOutlet NSBox           * fPictureCropBox;

    IBOutlet NSTextField     * fWidthField;
    IBOutlet NSStepper       * fWidthStepper;
    IBOutlet NSTextField     * fHeightField;
    IBOutlet NSStepper       * fHeightStepper;
    IBOutlet NSTextField     * fRatioLabel;
    IBOutlet NSButton        * fRatioCheck;
    IBOutlet NSMatrix        * fCropMatrix;
    IBOutlet NSTextField     * fCropTopField;
    IBOutlet NSStepper       * fCropTopStepper;
    IBOutlet NSTextField     * fCropBottomField;
    IBOutlet NSStepper       * fCropBottomStepper;
    IBOutlet NSTextField     * fCropLeftField;
    IBOutlet NSStepper       * fCropLeftStepper;
    IBOutlet NSTextField     * fCropRightField;
    IBOutlet NSStepper       * fCropRightStepper;

    IBOutlet NSTextField     * fModulusLabel;
    IBOutlet NSPopUpButton   * fModulusPopUp;

    IBOutlet NSTextField     * fDisplayWidthField;
    IBOutlet NSTextField     * fDisplayWidthLabel;

    IBOutlet NSTextField     * fParWidthField;
    IBOutlet NSTextField     * fParHeightField;
    IBOutlet NSTextField     * fParWidthLabel;
    IBOutlet NSTextField     * fParHeightLabel;

	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSTextField     * fSizeInfoField;

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
        NSArray *observerdKeyPaths = @[@"filters.detelecine", @"filters.detelecineCustomString",
                                       @"filters.deinterlace", @"filters.deinterlaceCustomString",
                                       @"filters.decomb", @"filters.decombCustomString",
                                       @"filters.denoise", @"filters.denoisePreset",
                                       @"filters.denoiseTune", @"filters.denoiseCustomString",
                                       @"filters.deblock", @"filters.grayscale", @"filters.useDecomb"];
        for (NSString *keyPath in observerdKeyPaths)
        {
            [self addObserver:self forKeyPath:keyPath options:NSKeyValueObservingOptionInitial context:HBPictureControllerContext];
        }

        fPreviewController = [[HBPreviewController alloc] init];
    }

	return self;
}

- (void) dealloc
{
    NSArray *observerdKeyPaths = @[@"filters.detelecine", @"filters.detelecineCustomString",
                                   @"filters.deinterlace", @"filters.deinterlaceCustomString",
                                   @"filters.decomb", @"filters.decombCustomString",
                                   @"filters.denoise", @"filters.denoisePreset",
                                   @"filters.denoiseTune", @"filters.denoiseCustomString",
                                   @"filters.deblock", @"filters.grayscale", @"filters.useDecomb"];
    @try {
        for (NSString *keyPath in observerdKeyPaths)
        {
            [self removeObserver:self forKeyPath:keyPath];
        }

    } @catch (NSException * __unused exception) {}

    [_filters release];
    [fPreviewController release];
    [super dealloc];
}

- (void)windowDidLoad
{
    [[self window] setExcludedFromWindowsMenu:YES];

    /* Populate the user interface */
    [fWidthStepper  setValueWraps: NO];
    [fHeightStepper setValueWraps: NO];

    [fCropTopStepper    setIncrement: 2];
    [fCropTopStepper    setMinValue:  0];
    [fCropBottomStepper setIncrement: 2];
    [fCropBottomStepper setMinValue:  0];
    [fCropLeftStepper   setIncrement: 2];
    [fCropLeftStepper   setMinValue:  0];
    [fCropRightStepper  setIncrement: 2];
    [fCropRightStepper  setMinValue:  0];

    /* Populate the Anamorphic NSPopUp button here */
    [fAnamorphicPopUp removeAllItems];
    [fAnamorphicPopUp addItemsWithTitles:@[@"None", @"Strict", @"Loose", @"Custom"]];

    /* populate the modulus popup here */
    [fModulusPopUp removeAllItems];
    [fModulusPopUp addItemsWithTitles:@[@"16", @"8", @"4", @"2"]];

    [self resizeInspectorForTab:nil];
    [self adjustSizingDisplay:nil];
}

- (void) setHandle: (hb_handle_t *) handle
{
    [fPreviewController setHandle: handle];
    [fPreviewController setDelegate:(HBController *)self.delegate];
}

- (void) setTitle: (hb_title_t *) title
{
    fTitle = title;

    if (!title) {
        [fPreviewController setTitle:NULL];
        return;
    }

    hb_job_t * job = title->job;

    fTitle = title;

    [fAnamorphicPopUp selectItemAtIndex: job->anamorphic.mode];
    if (job->anamorphic.mode == HB_ANAMORPHIC_STRICT)
    {
        [fWidthStepper  setEnabled: NO];
        [fHeightStepper setEnabled: NO];
        [fWidthField    setEditable:NO];
        [fHeightField   setEditable:NO];
    }
    else
    {
        [fWidthStepper  setEnabled: YES];
        [fHeightStepper setEnabled: YES];
        [fWidthField    setEditable:YES];
        [fHeightField   setEditable:YES];
    }
    if (job->anamorphic.mode == HB_ANAMORPHIC_STRICT ||
        job->anamorphic.mode == HB_ANAMORPHIC_LOOSE)
    {
        job->anamorphic.keep_display_aspect = 1;
        [fRatioCheck    setState:   NSOnState];
        [fRatioCheck    setEnabled: NO];
    }
    else
    {
        [fRatioCheck    setEnabled: YES];
        [fRatioCheck setState:   job->anamorphic.keep_display_aspect ?
                                                        NSOnState : NSOffState];
    }
    [fParWidthField setEnabled:     !job->anamorphic.keep_display_aspect];
    [fParHeightField setEnabled:    !job->anamorphic.keep_display_aspect];
    [fDisplayWidthField setEnabled: !job->anamorphic.keep_display_aspect];

    if (job->modulus)
    {
        [fModulusPopUp selectItemWithTitle: [NSString stringWithFormat:@"%d",job->modulus]];
        [fWidthStepper  setIncrement: job->modulus];
        [fHeightStepper setIncrement: job->modulus];
    }
    else
    {
        [fModulusPopUp selectItemAtIndex: 0];
        [fWidthStepper  setIncrement: 16];
        [fHeightStepper setIncrement: 16];
    }

    if (self.autoCrop)
    {
        [fCropMatrix  selectCellAtRow: 0 column:0];

        /* If auto, lets set the crop steppers according to
         * current fTitle->crop values */
        memcpy( job->crop, fTitle->crop, 4 * sizeof( int ) );
        [fCropTopStepper    setIntValue: fTitle->crop[0]];
        [fCropTopField      setIntValue: fTitle->crop[0]];
        [fCropBottomStepper setIntValue: fTitle->crop[1]];
        [fCropBottomField   setIntValue: fTitle->crop[1]];
        [fCropLeftStepper   setIntValue: fTitle->crop[2]];
        [fCropLeftField     setIntValue: fTitle->crop[2]];
        [fCropRightStepper  setIntValue: fTitle->crop[3]];
        [fCropRightField    setIntValue: fTitle->crop[3]];
    }
    else
    {
        [fCropMatrix  selectCellAtRow: 1 column:0];
        [fCropTopStepper    setIntValue: job->crop[0]];
        [fCropTopField      setIntValue: job->crop[0]];
        [fCropBottomStepper setIntValue: job->crop[1]];
        [fCropBottomField   setIntValue: job->crop[1]];
        [fCropLeftStepper   setIntValue: job->crop[2]];
        [fCropLeftField     setIntValue: job->crop[2]];
        [fCropRightStepper  setIntValue: job->crop[3]];
        [fCropRightField    setIntValue: job->crop[3]];
    }

    [fCropTopStepper    setEnabled: !self.autoCrop];
    [fCropBottomStepper setEnabled: !self.autoCrop];
    [fCropLeftStepper   setEnabled: !self.autoCrop];
    [fCropRightStepper  setEnabled: !self.autoCrop];

    [fCropTopField      setEditable: !self.autoCrop];
    [fCropBottomField   setEditable: !self.autoCrop];
    [fCropLeftField     setEditable: !self.autoCrop];
    [fCropRightField    setEditable: !self.autoCrop];

    [fWidthStepper      setMaxValue: title->width - job->crop[2] - job->crop[3]];
    [fWidthStepper      setIntValue: job->width];
    [fWidthField        setIntValue: job->width];
    [fHeightStepper     setMaxValue: title->height - job->crop[0] - job->crop[1]];
    [fHeightStepper     setIntValue: job->height];
    [fHeightField       setIntValue: job->height];
    [fCropTopStepper    setMaxValue: title->height/2-2];
    [fCropBottomStepper setMaxValue: title->height/2-2];
    [fCropLeftStepper   setMaxValue: title->width/2-2];
    [fCropRightStepper  setMaxValue: title->width/2-2];

    [fParWidthField     setIntValue: job->anamorphic.par_width];
    [fParHeightField    setIntValue: job->anamorphic.par_height];

    int display_width;
    display_width = job->width * job->anamorphic.par_width /
                                 job->anamorphic.par_height;
    [fDisplayWidthField setIntValue: display_width];

    [fPreviewController setTitle:title];

    [self settingsChanged:nil];
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBPictureControllerContext)
    {
        // We use KVO to update the panel
        // and notify the main controller of the changes
        // in the filters settings.
        if ([keyPath isEqualToString:@"filters.useDecomb"])
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
        else if ([keyPath isEqualToString:@"filters.deblock"])
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
        else if ([keyPath isEqualToString:@"filters.deinterlace"] || [keyPath isEqualToString:@"filters.decomb"])
        {
            // Might need to update the preview images with
            // the new deinterlace/decomb setting.
            if ((self.filters.deinterlace && !self.filters.useDecomb) ||
                (self.filters.decomb && self.filters.useDecomb))
            {
                fPreviewController.deinterlacePreview = YES;
            }
            else
            {
                fPreviewController.deinterlacePreview = NO;
            }
            [fPreviewController reload];
        }
        else if ([keyPath isEqualToString:@"filters.denoise"] || [keyPath isEqualToString:@"filters.denoisePreset"])
        {
            [self validateDenoiseUI];
        }

        // If one of the filters properties changes
        // update the UI in the main window
        if ([keyPath hasPrefix:@"filters"])
        {
            [self.delegate pictureSettingsDidChange];
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

        /* Set visibility of capuj widgets */
        [fParWidthField setHidden: NO];
        [fParHeightField setHidden: NO];
        [fParWidthLabel setHidden: NO];
        [fParHeightLabel setHidden: NO];
        [fDisplayWidthField setHidden: NO];
        [fDisplayWidthLabel setHidden: NO];
    }
    else
    {
        pictureSizingBoxSize.width = 200;

        /* Set visibility of capuj widgets */
        [fParWidthField setHidden: YES];
        [fParHeightField setHidden: YES];
        [fParWidthLabel setHidden: YES];
        [fParHeightLabel setHidden: YES];
        [fDisplayWidthField setHidden: YES];
        [fDisplayWidthLabel setHidden: YES];
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

- (NSString *) pictureSizeInfoString
{
    return [fSizeInfoField stringValue];
}

#pragma mark -

/**
 * Displays and brings the picture window to the front
 */
- (IBAction) showPictureWindow: (id) sender
{
    if ([[self window] isVisible])
    {
        [[self window] close];
    }
    else
    {
        [self showWindow:sender];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PictureSizeWindowIsOpen"];
        /* Set the window back to Floating Window mode
         * This will put the window always on top, but
         * since we have Hide on Deactivate set in our
         * xib, if other apps are put in focus we will
         * hide properly to stay out of the way
         */
        [[self window] setLevel:NSFloatingWindowLevel];
    }

    [self resizeInspectorForTab:nil];
    [self adjustSizingDisplay:nil];
}

- (IBAction) showPreviewWindow: (id) sender
{
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PreviewWindowIsOpen"];
    [fPreviewController showWindow:sender];
}

- (void) windowWillClose: (NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PictureSizeWindowIsOpen"];
}

/**
 * This method is used to detect clicking on a tab in fSizeFilterView
 */
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    [self resizeInspectorForTab:nil];
}

#pragma mark - Picture Update Logic

- (IBAction) settingsChanged: (id) sender
{
    if (!fTitle)
        return;

    hb_job_t * job = fTitle->job;
    int keep = 0, dar_updated = 0;

    if (sender == fAnamorphicPopUp)
    {
        job->anamorphic.mode = (int)[fAnamorphicPopUp indexOfSelectedItem];
        if (job->anamorphic.mode == HB_ANAMORPHIC_STRICT)
        {
            [fModulusLabel  setHidden:  YES];
            [fModulusPopUp  setHidden:  YES];
            [fWidthStepper  setEnabled: NO];
            [fHeightStepper setEnabled: NO];
            [fWidthField    setEditable:NO];
            [fHeightField   setEditable:NO];
        }
        else
        {
            [fModulusLabel  setHidden:  NO];
            [fModulusPopUp  setHidden:  NO];
            [fWidthStepper  setEnabled: YES];
            [fHeightStepper setEnabled: YES];
            [fWidthField    setEditable:YES];
            [fHeightField    setEditable:YES];
        }
        if (job->anamorphic.mode == HB_ANAMORPHIC_STRICT ||
            job->anamorphic.mode == HB_ANAMORPHIC_LOOSE)
        {
            job->anamorphic.keep_display_aspect = 1;
            [fRatioCheck setState: NSOnState];
            [fRatioCheck setEnabled: NO];
        }
        else
        {
            [fRatioCheck setEnabled: YES];
            [fRatioCheck setState:   job->anamorphic.keep_display_aspect ?
                                                        NSOnState : NSOffState];
        }
    }
    else if (sender == fModulusPopUp)
    {
        job->modulus = [[fModulusPopUp titleOfSelectedItem] intValue];
        [fWidthStepper  setIncrement: job->modulus];
        [fHeightStepper setIncrement: job->modulus];
    }

    if (sender == fRatioCheck)
    {
        job->anamorphic.keep_display_aspect = [fRatioCheck  state] == NSOnState;
        [fParWidthField setEnabled:     !job->anamorphic.keep_display_aspect];
        [fParHeightField setEnabled:    !job->anamorphic.keep_display_aspect];
        [fDisplayWidthField setEnabled: !job->anamorphic.keep_display_aspect];
    }

    if (sender == fHeightStepper || sender == fHeightField)
    {
        keep |= HB_KEEP_HEIGHT;

        if (sender == fHeightStepper)
            job->height = [fHeightStepper intValue];
        else
            job->height = [fHeightField intValue];
    }

    if (sender == fWidthStepper || sender == fWidthField)
    {
        keep |= HB_KEEP_WIDTH;

        if (sender == fWidthStepper)
            job->width = [fWidthStepper intValue];
        else
            job->width = [fWidthField intValue];
    }

    if (sender == fParWidthField || sender == fParHeightField)
    {
        job->anamorphic.par_width = [fParWidthField intValue];
        job->anamorphic.par_height = [fParHeightField intValue];
    }

    if (sender == fDisplayWidthField)
    {
        dar_updated = 1;
        job->anamorphic.dar_width = [fDisplayWidthField intValue];
        job->anamorphic.dar_height = [fHeightStepper intValue];
    }

    if (sender == fCropMatrix)
    {
        if (self.autoCrop != ( [fCropMatrix selectedRow] == 0 ))
        {
            self.autoCrop = !self.autoCrop;
            if (self.autoCrop)
            {
                /* If auto, lets set the crop steppers according to
                 * current fTitle->crop values */
                memcpy( job->crop, fTitle->crop, 4 * sizeof( int ) );
                [fCropTopStepper    setIntValue: fTitle->crop[0]];
                [fCropTopField      setIntValue: fTitle->crop[0]];
                [fCropBottomStepper setIntValue: fTitle->crop[1]];
                [fCropBottomField   setIntValue: fTitle->crop[1]];
                [fCropLeftStepper   setIntValue: fTitle->crop[2]];
                [fCropLeftField     setIntValue: fTitle->crop[2]];
                [fCropRightStepper  setIntValue: fTitle->crop[3]];
                [fCropRightField    setIntValue: fTitle->crop[3]];
            }
            [fCropTopStepper    setEnabled: !self.autoCrop];
            [fCropBottomStepper setEnabled: !self.autoCrop];
            [fCropLeftStepper   setEnabled: !self.autoCrop];
            [fCropRightStepper  setEnabled: !self.autoCrop];

            [fCropTopField      setEditable: !self.autoCrop];
            [fCropBottomField   setEditable: !self.autoCrop];
            [fCropLeftField     setEditable: !self.autoCrop];
            [fCropRightField    setEditable: !self.autoCrop];
        }
    }
    if (sender == fCropTopStepper)
    {
        job->crop[0] = [fCropTopStepper    intValue];
        [fCropTopField setIntValue: job->crop[0]];
        [fHeightStepper setMaxValue: fTitle->height - job->crop[0] - job->crop[1]];
    }
    if (sender == fCropBottomStepper)
    {
        job->crop[1] = [fCropBottomStepper intValue];
        [fCropBottomField setIntValue: job->crop[1]];
        [fHeightStepper setMaxValue: fTitle->height - job->crop[0] - job->crop[1]];
    }
    if (sender == fCropLeftStepper)
    {
        job->crop[2] = [fCropLeftStepper   intValue];
        [fCropLeftField setIntValue: job->crop[2]];
        [fWidthStepper setMaxValue: fTitle->width - job->crop[2] - job->crop[3]];
    }
    if (sender == fCropRightStepper)
    {
        job->crop[3] = [fCropRightStepper  intValue];
        [fCropRightField setIntValue: job->crop[3]];
        [fWidthStepper setMaxValue: fTitle->width - job->crop[2] - job->crop[3]];
    }

    if (sender == fCropTopField)
    {
        int cropValue = [fCropTopField intValue];
        if (cropValue >= 0 && (cropValue <= fTitle->height/2-2))
        {
            job->crop[0] = cropValue;
            [fCropTopStepper setIntValue:cropValue];
            [fHeightStepper setMaxValue: fTitle->height - job->crop[0] - job->crop[1]];
        }
        else
        {
            [fCropLeftField setIntValue:job->crop[0]];
        }
    }
    else if (sender == fCropBottomField)
    {
        int cropValue = [fCropBottomField intValue];
        if (cropValue >= 0 && (cropValue <= fTitle->height/2-2))
        {
            job->crop[1] = cropValue;
            [fCropBottomStepper setIntValue:cropValue];
            [fHeightStepper setMaxValue: fTitle->height - job->crop[0] - job->crop[1]];
        }
        else
        {
            [fCropLeftField setIntValue:job->crop[1]];
        }
    }
    else if (sender == fCropLeftField)
    {
        int cropValue = [fCropLeftField intValue];
        if (cropValue >= 0 && (cropValue <= fTitle->width/2-2))
        {
            job->crop[2] = cropValue;
            [fCropLeftStepper setIntValue:cropValue];
            [fWidthStepper setMaxValue: fTitle->width - job->crop[2] - job->crop[3]];
        }
        else
        {
            [fCropLeftField setIntValue:job->crop[2]];
        }
    }
    else if (sender == fCropRightField)
    {
        int cropValue = [fCropRightField intValue];
        if (cropValue >= 0 && (cropValue <= fTitle->width/2-2))
        {
            job->crop[3] = cropValue;
            [fCropRightStepper setIntValue:cropValue];
            [fWidthStepper setMaxValue: fTitle->width - job->crop[2] - job->crop[3]];
        }
        else
        {
            [fCropLeftField setIntValue:job->crop[3]];
        }
    }

    keep |= !!job->anamorphic.keep_display_aspect * HB_KEEP_DISPLAY_ASPECT;

    hb_geometry_t srcGeo, resultGeo;
    hb_ui_geometry_t uiGeo;

    srcGeo.width = fTitle->width;
    srcGeo.height = fTitle->height;
    srcGeo.par.num = fTitle->pixel_aspect_width;
    srcGeo.par.den = fTitle->pixel_aspect_height;

    uiGeo.mode = job->anamorphic.mode;
    uiGeo.keep = keep;
    uiGeo.itu_par = 0;
    uiGeo.modulus = job->modulus;
    memcpy(uiGeo.crop, job->crop, sizeof(int[4]));
    uiGeo.width = job->width;
    uiGeo.height =  job->height;
    /* Modulus added to maxWidth/maxHeight to allow a small amount of
     * upscaling to the next mod boundary.
     */
    uiGeo.maxWidth = fTitle->width - job->crop[2] - job->crop[3] + job->modulus - 1;
    uiGeo.maxHeight = fTitle->height - job->crop[0] - job->crop[1] + job->modulus - 1;
    uiGeo.par.num = job->anamorphic.par_width;
    uiGeo.par.den = job->anamorphic.par_height;
    uiGeo.dar.num = 0;
    uiGeo.dar.den = 0;
    if (job->anamorphic.mode == HB_ANAMORPHIC_CUSTOM && dar_updated)
    {
        uiGeo.dar.num = job->anamorphic.dar_width;
        uiGeo.dar.den = job->anamorphic.dar_height;
    }
    hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);

    job->width = resultGeo.width;
    job->height = resultGeo.height;
    job->anamorphic.par_width = resultGeo.par.num;
    job->anamorphic.par_height = resultGeo.par.den;

    int display_width;
    display_width = resultGeo.width * resultGeo.par.num / resultGeo.par.den;

    [fWidthStepper      setIntValue: resultGeo.width];
    [fWidthField        setIntValue: resultGeo.width];
    [fHeightStepper     setIntValue: resultGeo.height];
    [fHeightField       setIntValue: resultGeo.height];
    [fParWidthField     setIntValue: resultGeo.par.num];
    [fParHeightField    setIntValue: resultGeo.par.den];
    [fDisplayWidthField setIntValue: display_width];

    /*
     * Sanity-check here for previews < 16 pixels to avoid crashing
     * hb_get_preview(). In fact, let's get previews at least 64 pixels in both
     * dimensions; no human can see any meaningful detail below that.
     */
    if (job->width >= 64 && job->height >= 64)
    {
        [fPreviewController reload];
    }

    /* we get the sizing info to display from fPreviewController */
    [fSizeInfoField setStringValue: [fPreviewController pictureSizeInfoString]];

    if (sender != nil)
    {
        [self.delegate pictureSettingsDidChange];
    }

    if ([[self window] isVisible])
    {
        [self adjustSizingDisplay:nil];
    }
}

@end
