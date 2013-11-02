/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "Controller.h"
#import "PictureController.h"
#import "HBPreviewController.h"

@interface HBPictureController ()
{
    hb_title_t               * fTitle;

    PreviewController        * fPreviewController;

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

    /* for now we setup some values to remember our pars and dars
     * from scan
     */
    int titleParWidth;
    int titleParHeight;
    float dar;

	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSTextField     * fSizeInfoField;

    int output_width, output_height, output_par_width, output_par_height;
    int display_width;

    /* used to track the previous state of the keep aspect
     ratio checkbox when turning anamorphic on, so it can be
     returned to the previous state when anamorphic is turned
     off */
    BOOL    keepAspectRatioPreviousState;

    /* Video Filters */
    IBOutlet NSBox           * fDetelecineBox;
    IBOutlet NSPopUpButton   * fDetelecinePopUp;

    IBOutlet NSBox           * fDecombDeinterlaceBox;
    IBOutlet NSSlider        * fDecombDeinterlaceSlider;

    IBOutlet NSBox           * fDecombBox;
    IBOutlet NSPopUpButton   * fDecombPopUp;

    IBOutlet NSBox           * fDeinterlaceBox;
    IBOutlet NSPopUpButton   * fDeinterlacePopUp;

    IBOutlet NSBox           * fDenoiseBox;
    IBOutlet NSPopUpButton   * fDenoisePopUp;

    IBOutlet NSBox           * fDeblockBox; // also holds the grayscale box
    IBOutlet NSTextField     * fDeblockField;
    IBOutlet NSSlider        * fDeblockSlider;

    IBOutlet NSButton        * fGrayscaleCheck;
}

- (void) tabView: (NSTabView *) tabView didSelectTabViewItem: (NSTabViewItem *) tabViewItem;

- (void) resizeInspectorForTab: (id) sender;

- (void) adjustSizingDisplay:(id) sender;
- (void) adjustFilterDisplay: (id) sender;

- (void) reloadStillPreview;

/* Internal Actions */
- (IBAction) settingsChanged: (id) sender;
- (IBAction) FilterSettingsChanged: (id) sender;
- (IBAction) modeDecombDeinterlaceSliderChanged: (id) sender;
- (IBAction) deblockSliderChanged: (id) sender;

@end

@implementation HBPictureController

- (id) init
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

        _detelecineCustomString = @"";
        _deinterlaceCustomString = @"";
        _decombCustomString = @"";
        _denoiseCustomString = @"";

        fPreviewController = [[PreviewController alloc] init];
    }

	return self;
}

- (void) awakeFromNib
{
    [[self window] setDelegate:self];

    if( ![[self window] setFrameUsingName:@"PictureSizing"] )
        [[self window] center];

    [self setWindowFrameAutosaveName:@"PictureSizing"];
    [[self window] setExcludedFromWindowsMenu:YES];

    /* Populate the user interface */
    [fWidthStepper  setValueWraps: NO];
    [fWidthStepper  setIncrement: 16];
    [fWidthStepper  setMinValue: 64];
    [fHeightStepper setValueWraps: NO];
    [fHeightStepper setIncrement: 16];
    [fHeightStepper setMinValue: 64];

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

    /* we use a popup to show the detelecine settings */
    [fDetelecinePopUp removeAllItems];
    [fDetelecinePopUp addItemsWithTitles:@[@"Off", @"Custom", @"Default"]];
    [fDetelecinePopUp selectItemAtIndex: self.detelecine];

    /* we use a popup to show the decomb settings */
	[fDecombPopUp removeAllItems];
    [fDecombPopUp addItemsWithTitles:@[@"Off", @"Custom", @"Default", @"Fast", @"Bob"]];
    [self modeDecombDeinterlaceSliderChanged:nil];
    [fDecombPopUp selectItemAtIndex: self.decomb];

    /* we use a popup to show the deinterlace settings */
	[fDeinterlacePopUp removeAllItems];
    [fDeinterlacePopUp addItemsWithTitles:@[@"Off", @"Custom", @"Fast", @"Slow", @"Slower", @"Bob"]];
    [fDeinterlacePopUp selectItemAtIndex: self.deinterlace];

    /* we use a popup to show the denoise settings */
	[fDenoisePopUp removeAllItems];
    [fDenoisePopUp addItemsWithTitles:@[@"Off", @"Custom", @"Weak", @"Medium", @"Strong"]];
    [fDenoisePopUp selectItemAtIndex: self.denoise];
}

- (void) setHandle: (hb_handle_t *) handle
{
    [fPreviewController SetHandle: handle];
    [fPreviewController setHBController:(HBController *)self.delegate];
}

- (void) setTitle: (hb_title_t *) title
{
    hb_job_t * job = title->job;

    fTitle = title;

    [fWidthStepper      setMaxValue: title->width];
    [fWidthStepper      setIntValue: job->width];
    [fWidthField        setIntValue: job->width];
    [fHeightStepper     setMaxValue: title->height];
    [fHeightStepper     setIntValue: job->height];
    [fHeightField       setIntValue: job->height];
    [fRatioCheck        setState:    job->keep_ratio ? NSOnState : NSOffState];
    [fCropTopStepper    setMaxValue: title->height/2-2];
    [fCropBottomStepper setMaxValue: title->height/2-2];
    [fCropLeftStepper   setMaxValue: title->width/2-2];
    [fCropRightStepper  setMaxValue: title->width/2-2];

    [fAnamorphicPopUp selectItemAtIndex: job->anamorphic.mode];

    if (job->modulus)
    {
        [fModulusPopUp selectItemWithTitle: [NSString stringWithFormat:@"%d",job->modulus]];
    }
    else
    {
        [fModulusPopUp selectItemAtIndex: 0];
    }

    /* We initially set the previous state of keep ar to on */
    keepAspectRatioPreviousState = 1;
	if (!self.autoCrop)
	{
        [fCropMatrix  selectCellAtRow: 1 column:0];
        /* If auto, lets set the crop steppers according to current job->crop values */
        [fCropTopStepper    setIntValue: job->crop[0]];
        [fCropTopField      setIntValue: job->crop[0]];
        [fCropBottomStepper setIntValue: job->crop[1]];
        [fCropBottomField   setIntValue: job->crop[1]];
        [fCropLeftStepper   setIntValue: job->crop[2]];
        [fCropLeftField     setIntValue: job->crop[2]];
        [fCropRightStepper  setIntValue: job->crop[3]];
        [fCropRightField    setIntValue: job->crop[3]];
	}
	else
	{
        [fCropMatrix  selectCellAtRow: 0 column:0];
	}

    /* Set filters widgets according to the filters struct */
    [fDetelecinePopUp selectItemAtIndex:self.detelecine];
    [fDecombPopUp selectItemAtIndex:self.decomb];
    [fDeinterlacePopUp selectItemAtIndex: self.deinterlace];
    [fDenoisePopUp selectItemAtIndex: self.denoise];
    [fDeblockSlider setFloatValue:self.deblock];
    [fGrayscaleCheck setState:self.grayscale];

    [self deblockSliderChanged: nil];

    titleParWidth = job->anamorphic.par_width;
    titleParHeight = job->anamorphic.par_height;

    [fPreviewController SetTitle:title];

    [self settingsChanged:nil];
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
        frame.size.width = 314;
        /* we glean the height from the size of the boxes plus the extra window space
         * needed for non boxed display
         */
        frame.size.height = 110.0 + [fDetelecineBox frame].size.height + [fDecombDeinterlaceBox frame].size.height + [fDenoiseBox frame].size.height + [fDeblockBox frame].size.height;
        /* Hide the size readout at the bottom as the vertical inspector is not wide enough */
        [fSizeInfoField setHidden:YES];
    }
    else // we are Tab index 0 which is size
    {
        frame.size.width = 30.0 + [fPictureSizeBox frame].size.width + [fPictureCropBox frame].size.width;
        frame.size.height = [fPictureSizeBox frame].size.height + 90;
        /* hide the size summary field at the bottom */
        [fSizeInfoField setHidden:NO];
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

    if ([fAnamorphicPopUp indexOfSelectedItem] == 3) // custom / power user jamboree
    {
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
    if (pictureSizingBoxSize.height != [fPictureSizeBox frame].size.height || pictureSizingBoxSize.width != [fPictureSizeBox frame].size.width)
    {
        /* Get our delta for the change in picture size box height */
        CGFloat deltaYSizeBoxShift = pictureSizingBoxSize.height - [fPictureSizeBox frame].size.height;
        fPictureSizeBoxOrigin.y -= deltaYSizeBoxShift;
        /* Get our delta for the change in picture size box width */
        CGFloat deltaXSizeBoxShift = pictureSizingBoxSize.width - [fPictureSizeBox frame].size.width;
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

- (void) adjustFilterDisplay: (id) sender
{
    NSBox *filterBox = nil;
    if (sender == fDetelecinePopUp)
    {
        filterBox = fDetelecineBox;
    }

    if (sender == fDecombDeinterlaceSlider)
    {
        if ([fDecombDeinterlaceSlider floatValue] == 0.0)
        {
            filterBox = fDecombBox;
        }
        else
        {
            filterBox = fDeinterlaceBox;
        }
    }

    if (sender == fDecombPopUp)
    {
        filterBox = fDecombBox;
    }

    if (sender == fDeinterlacePopUp)
    {
        filterBox = fDeinterlaceBox;
    }

    if (sender == fDenoisePopUp)
    {
        filterBox = fDenoiseBox;
    }

    NSSize currentSize = [filterBox frame].size;
    NSRect boxFrame = [filterBox frame];

    if ([[sender titleOfSelectedItem]  isEqualToString: @"Custom"])
    {
        currentSize.height = 60;
    }
    else
    {
        currentSize.height = 36;
    }

    /* Check to see if we have changed the size from current */
    if (currentSize.height != [filterBox frame].size.height)
    {
        /* We are changing the size of the box, so recalc the origin */
        NSPoint boxOrigin = [filterBox frame].origin;
        /* We get the deltaY here for how much we are expanding/contracting the box vertically */
        CGFloat deltaYBoxShift = currentSize.height - [filterBox frame].size.height;
        boxOrigin.y -= deltaYBoxShift;

        boxFrame.size.height = currentSize.height;
        boxFrame.origin.y = boxOrigin.y;
        [filterBox setFrame:boxFrame];

        if (filterBox == fDecombBox || filterBox == fDeinterlaceBox)
        {
            /* fDecombDeinterlaceBox*/
            NSSize decombDeinterlaceBoxSize = [fDecombDeinterlaceBox frame].size;
            NSPoint decombDeinterlaceBoxOrigin = [fDecombDeinterlaceBox frame].origin;

            if ([fDeinterlaceBox isHidden] == YES)
            {
                decombDeinterlaceBoxSize.height = [fDecombBox frame].size.height + 50;
            }
            else
            {
                decombDeinterlaceBoxSize.height = [fDeinterlaceBox frame].size.height + 50;
            }
            /* get delta's for the change in window size */

            CGFloat deltaYdecombDeinterlace = decombDeinterlaceBoxSize.height - [fDecombDeinterlaceBox frame].size.height;

            deltaYBoxShift = deltaYdecombDeinterlace;

            decombDeinterlaceBoxOrigin.y -= deltaYdecombDeinterlace;

            [fDecombDeinterlaceBox setFrameSize:decombDeinterlaceBoxSize];
            [fDecombDeinterlaceBox setFrameOrigin:decombDeinterlaceBoxOrigin];
        }

        /* now we must reset the origin of each box below the adjusted box*/
        NSPoint decombDeintOrigin = [fDecombDeinterlaceBox frame].origin;
        NSPoint denoiseOrigin = [fDenoiseBox frame].origin;
        NSPoint deblockOrigin = [fDeblockBox frame].origin;
        if (sender == fDetelecinePopUp)
        {
            decombDeintOrigin.y -= deltaYBoxShift;
            [fDecombDeinterlaceBox setFrameOrigin:decombDeintOrigin];

            denoiseOrigin.y -= deltaYBoxShift;
            [fDenoiseBox setFrameOrigin:denoiseOrigin];

            deblockOrigin.y -= deltaYBoxShift;
            [fDeblockBox setFrameOrigin:deblockOrigin];
        }
        if (sender == fDecombPopUp || sender == fDeinterlacePopUp)
        {
            denoiseOrigin.y -= deltaYBoxShift;
            [fDenoiseBox setFrameOrigin:denoiseOrigin];

            deblockOrigin.y -= deltaYBoxShift;
            [fDeblockBox setFrameOrigin:deblockOrigin];
        }

        if (sender == fDenoisePopUp)
        {
            deblockOrigin.y -= deltaYBoxShift;
            [fDeblockBox setFrameOrigin:deblockOrigin];
        }

        /* now we call to resize the entire inspector window */
        [self resizeInspectorForTab:nil];
    }
}

- (NSString *) pictureSizeInfoString
{
    return [fSizeInfoField stringValue];
}

- (void) reloadStillPreview
{
    [fPreviewController SetTitle:fTitle];
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

    [self adjustFilterDisplay:nil];
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

- (BOOL) windowShouldClose: (id) sender
{
    return YES;
}

/**
 * This method is used to detect clicking on a tab in fSizeFilterView
 */
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    [self resizeInspectorForTab:nil];
}

- (void) dealloc
{
    [fPreviewController release];
    [super dealloc];
}

#pragma mark -
#pragma mark Interface Update Logic

- (IBAction) settingsChanged: (id) sender
{
    hb_job_t * job = fTitle->job;

    /* if we are anything but strict anamorphic */
    if ([fAnamorphicPopUp indexOfSelectedItem] != 1)
    {
        [fModulusLabel setHidden:NO];
        [fModulusPopUp setHidden:NO];
        if (sender == fModulusPopUp)
        {
            /* do a dry run with hb_fix aspect to get new modulus */
            job->modulus = [[fModulusPopUp titleOfSelectedItem] intValue];
            job->keep_ratio  = 1;
            hb_fix_aspect( job, HB_KEEP_WIDTH );
            if( job->height > fTitle->height )
            {
                job->height = fTitle->height;
                hb_fix_aspect( job, HB_KEEP_HEIGHT );
            }
            [fWidthStepper      setIntValue: job->width];
            [fWidthField        setIntValue: job->width];
            if( [fAnamorphicPopUp indexOfSelectedItem] != 2) // if we are not loose or custom
            {
                [fHeightStepper     setIntValue: job->height];
                [fHeightField       setIntValue: job->height];
            }
        }
    }
    else
    {
        /* we are strict so hide the mod popup since libhb uses mod 2 for strict anamorphic*/
        [fModulusLabel setHidden:YES];
        [fModulusPopUp setHidden:YES];
    }

    job->modulus = [[fModulusPopUp titleOfSelectedItem] intValue];

    [fWidthStepper  setIncrement: job->modulus];
    [fHeightStepper setIncrement: job->modulus];

    /* Since custom anamorphic allows for a height setting > fTitle->height
     * check to make sure it is returned to fTitle->height for all other modes
     */
    [fHeightStepper setMaxValue: fTitle->height];

    self.autoCrop = ( [fCropMatrix selectedRow] == 0 );
    [fCropTopStepper    setEnabled: !self.autoCrop];
    [fCropBottomStepper setEnabled: !self.autoCrop];
    [fCropLeftStepper   setEnabled: !self.autoCrop];
    [fCropRightStepper  setEnabled: !self.autoCrop];

    if( self.autoCrop )
    {
        memcpy( job->crop, fTitle->crop, 4 * sizeof( int ) );
    }
    else
    {
        job->crop[0] = [fCropTopStepper    intValue];
        job->crop[1] = [fCropBottomStepper intValue];
        job->crop[2] = [fCropLeftStepper   intValue];
        job->crop[3] = [fCropRightStepper  intValue];
    }

    [fRatioCheck setEnabled: YES];

    [fParWidthField setEnabled: NO];
    [fParHeightField setEnabled: NO];
    [fDisplayWidthField setEnabled: NO];

    /* If we are not custom anamorphic, make sure we retain the orginal par */
    if( [fAnamorphicPopUp indexOfSelectedItem] != 3 )
    {
        job->anamorphic.par_width = titleParWidth;
        job->anamorphic.par_height = titleParHeight;
        [fRatioLabel setHidden: NO];
    }

	if( [fAnamorphicPopUp indexOfSelectedItem] > 0 )
	{
        if ([fAnamorphicPopUp indexOfSelectedItem] == 1) // strict
        {
            [fWidthStepper      setIntValue: fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3]];
            [fWidthField        setIntValue: fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3]];

            /* This will show correct anamorphic height values, but
             show distorted preview picture ratio */
            [fHeightStepper      setIntValue: fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1]];
            [fHeightField        setIntValue: fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1]];
            job->width       = [fWidthStepper  intValue];
            job->height      = [fHeightStepper intValue];

            job->anamorphic.mode = 1;
            [fWidthStepper setEnabled: NO];
            [fWidthField setEnabled: NO];
            [fHeightStepper setEnabled: NO];
            [fHeightField setEnabled: NO];
            [fRatioCheck setEnabled: NO];
        }
        else if ([fAnamorphicPopUp indexOfSelectedItem] == 2) // Loose anamorphic
        {
            job->anamorphic.mode = 2;
            [fWidthStepper setEnabled: YES];
            [fWidthField setEnabled: YES];
            [fRatioCheck setEnabled: NO];
            [fHeightStepper setEnabled: NO];
            [fHeightField setEnabled: NO];
            /* We set job->width and call hb_set_anamorphic_size in libhb to do a "dry run" to get
             * the values to be used by libhb for loose anamorphic
             */
            /* if the sender is the anamorphic popup, then we know that loose anamorphic has just
             * been turned on, so snap the width to full width for the source.
             */
            if (sender == fAnamorphicPopUp)
            {
                [fWidthStepper      setIntValue: fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3]];
                [fWidthField        setIntValue: fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3]];
            }
            job->width       = [fWidthStepper  intValue];
            hb_set_anamorphic_size(job, &output_width, &output_height, &output_par_width, &output_par_height);
            [fHeightStepper      setIntValue: output_height];
            [fHeightField        setIntValue: output_height];
            job->height      = [fHeightStepper intValue];

        }
        else if ([fAnamorphicPopUp indexOfSelectedItem] == 3) // custom / power user jamboree
        {
            job->anamorphic.mode = 3;

            /* Set the status of our custom ana only widgets accordingly */
            [fModulusPopUp setEnabled:YES];

            [fWidthStepper setEnabled: YES];
            [fWidthField setEnabled: YES];

            [fHeightStepper setEnabled: YES];
            /* for capuj the storage field is immaterial */
            [fHeightField setEnabled: YES];

            [fRatioCheck setEnabled: YES];
            if (sender == fRatioCheck)
            {
                if ([fRatioCheck  state] == NSOnState)
                {
                    [fParWidthField setEnabled: NO];
                    [fParHeightField setEnabled: NO];
                }
                else
                {
                    [fParWidthField setEnabled: YES];
                    [fParHeightField setEnabled: YES];
                }
            }

            [fParWidthField setEnabled: YES];
            [fParHeightField setEnabled: YES];

            [fDisplayWidthField setEnabled: YES];


            /* If we are coming into custom anamorphic we reset the par to original
             * which gives us a way back if things get hosed up.
             */

            if (sender == fAnamorphicPopUp)
            {
                /* When entering custom anamorphic, we start with keep ar on */
                [fRatioCheck  setState: NSOnState];
                /*
                 KEEPING ASPECT RATIO
                 Disable editing: PIXEL WIDTH, PIXEL HEIGHT
                 */
                [fParWidthField setEnabled: NO];
                [fParHeightField setEnabled: NO];

                job->width = [fWidthStepper intValue];
                job->height = [fHeightStepper intValue];

                /* make sure our par is set back to original */
                job->anamorphic.par_width = titleParWidth;
                job->anamorphic.par_height = titleParHeight;

                [fParWidthField   setIntValue: titleParWidth];
                [fParHeightField   setIntValue: titleParHeight];

                /* modify our par dims from our storage dims */
                hb_set_anamorphic_size(job, &output_width, &output_height, &output_par_width, &output_par_height);
                float par_display_width = (float)output_width * (float)output_par_width / (float)output_par_height;

                /* go ahead and mod the display dims */
                [fDisplayWidthField   setStringValue: [NSString stringWithFormat:@"%.2f", par_display_width]];

                job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                job->anamorphic.dar_height = (float)[fHeightStepper intValue];

                /* Set our dar here assuming we are just coming into capuj mode */
                dar = [fDisplayWidthField floatValue] / (float)[fHeightField intValue];

            }

            /* For capuj we disable these fields if we are keeping the dispay aspect */
            if ([fRatioCheck  state] == NSOnState)
            {
                /*
                 KEEPING ASPECT RATIO
                 DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification) */
                /*Disable editing: PIXEL WIDTH, PIXEL HEIGHT */

                [fParWidthField setEnabled: NO];
                [fParHeightField setEnabled: NO];

                /* Changing DISPLAY WIDTH: */
                if (sender == fDisplayWidthField)
                {
                    job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                    /* Changes HEIGHT to keep DAR */
                    /* calculate the height to retain the dar  */
                    int raw_calulated_height = (int)((int)[fDisplayWidthField floatValue] / dar);
                    /*  now use the modulus to go lower if there is a remainder  */
                    /* Note to me, raw_calulated_height % [[fModulusPopUp titleOfSelectedItem] intValue]
                     * gives me the remainder we are not mod (whatever our modulus is) subtract that from
                     * the actual calculated value derived from the dar to round down to the nearest mod value.
                     * This should be desireable over rounding up to the next mod value
                     */
                    int modulus_height = raw_calulated_height - (raw_calulated_height % [[fModulusPopUp titleOfSelectedItem] intValue]);
                    if (modulus_height > fTitle->height)
                    {
                        [fHeightStepper setMaxValue: modulus_height];
                    }
                    [fHeightStepper setIntValue: modulus_height];
                    job->anamorphic.dar_height = (float)[fHeightStepper intValue];
                    job->height = [fHeightStepper intValue];

                    /* Changes PIXEL WIDTH to new DISPLAY WIDTH */
                    [fParWidthField setIntValue: [fDisplayWidthField intValue]];
                    job->anamorphic.par_width = [fParWidthField intValue];
                    /* Changes PIXEL HEIGHT to STORAGE WIDTH */
                    [fParHeightField  setIntValue: [fWidthField intValue]];
                    job->anamorphic.par_height = [fParHeightField intValue];

                }
                /* Changing HEIGHT: */
                if (sender == fHeightStepper)
                {
                    job->anamorphic.dar_height = (float)[fHeightStepper intValue];
                    job->height = [fHeightStepper intValue];

                    /* Changes DISPLAY WIDTH to keep DAR*/
                    [fDisplayWidthField setStringValue: [NSString stringWithFormat: @"%.2f",[fHeightStepper intValue] * dar]];
                    job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                    /* Changes PIXEL WIDTH to new DISPLAY WIDTH */
                    [fParWidthField setIntValue: [fDisplayWidthField intValue]];
                    job->anamorphic.par_width = [fParWidthField intValue];
                    /* Changes PIXEL HEIGHT to STORAGE WIDTH */
                    [fParHeightField  setIntValue: [fWidthField intValue]];
                    job->anamorphic.par_height = [fParHeightField intValue];
                }
                /* Changing STORAGE_WIDTH: */
                if (sender == fWidthStepper)
                {
                    job->width = [fWidthStepper intValue];

                    job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                    job->anamorphic.dar_height = [fHeightStepper floatValue];

                    /* Changes PIXEL WIDTH to DISPLAY WIDTH */
                    [fParWidthField setIntValue: [fDisplayWidthField intValue]];
                    job->anamorphic.par_width = [fParWidthField intValue];
                    /* Changes PIXEL HEIGHT to new STORAGE WIDTH */
                    [fParHeightField  setIntValue: [fWidthStepper intValue]];
                    job->anamorphic.par_height = [fParHeightField intValue];
                }
            }
            else if ([fRatioCheck  state] == NSOffState)
            {
                /* Changing STORAGE_WIDTH: */
                if (sender == fWidthStepper)
                {
                    job->width = [fWidthStepper intValue];
                    /* changes DISPLAY WIDTH to STORAGE WIDTH * PIXEL WIDTH / PIXEL HEIGHT */
                    [fDisplayWidthField setStringValue: [NSString stringWithFormat: @"%.2f",(float)[fWidthStepper intValue] * [fParWidthField intValue] / [fParHeightField intValue]]];
                    job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                }
                /* Changing PIXEL dimensions */
                if (sender == fParWidthField || sender == fParHeightField)
                {
                    job->anamorphic.par_width = [fParWidthField intValue];
                    job->anamorphic.par_height = [fParHeightField intValue];
                    /* changes DISPLAY WIDTH to STORAGE WIDTH * PIXEL WIDTH / PIXEL HEIGHT */
                    [fDisplayWidthField setStringValue: [NSString stringWithFormat: @"%.2f",(float)[fWidthStepper intValue] * [fParWidthField intValue] / [fParHeightField intValue]]];
                    job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                }
                /* Changing DISPLAY WIDTH: */
                if (sender == fDisplayWidthField)
                {
                    job->anamorphic.dar_width = [fDisplayWidthField floatValue];
                    job->anamorphic.dar_height = (float)[fHeightStepper intValue];
                    /* changes PIXEL WIDTH to DISPLAY WIDTH and PIXEL HEIGHT to STORAGE WIDTH */
                    [fParWidthField setIntValue: [fDisplayWidthField intValue]];
                    job->anamorphic.par_width = [fParWidthField intValue];

                    [fParHeightField  setIntValue: [fWidthField intValue]];
                    job->anamorphic.par_height = [fParHeightField intValue];
                    hb_set_anamorphic_size(job, &output_width, &output_height, &output_par_width, &output_par_height);
                }
                /* Changing HEIGHT: */
                if (sender == fHeightStepper)
                {
                    /* just....changes the height.*/
                    job->anamorphic.dar_height = [fHeightStepper intValue];
                    job->height = [fHeightStepper intValue];
                }

            }
        }

        /* if the sender is the Anamorphic checkbox, record the state
         of KeepAspect Ratio so it can be reset if Anamorphic is unchecked again */
        if (sender == fAnamorphicPopUp)
        {
            keepAspectRatioPreviousState = [fRatioCheck state];
        }
        if ([fAnamorphicPopUp indexOfSelectedItem] != 3)
        {
            [fRatioCheck setState:NSOffState];
        }

    }
    else
	{
        job->width       = [fWidthStepper  intValue];
        job->height      = [fHeightStepper intValue];
        job->anamorphic.mode = 0;
        [fWidthStepper setEnabled: YES];
        [fWidthField setEnabled: YES];
        [fHeightStepper setEnabled: YES];
        [fHeightField setEnabled: YES];
        [fRatioCheck setEnabled: YES];
        /* if the sender is the Anamorphic checkbox, we return the
         keep AR checkbox to its previous state */
        if (sender == fAnamorphicPopUp)
        {
            [fRatioCheck setState:keepAspectRatioPreviousState];
        }
	}

    if ([fAnamorphicPopUp indexOfSelectedItem] != 3)
    {
        job->keep_ratio  = ( [fRatioCheck state] == NSOnState );
        if( job->keep_ratio )
        {
            if( sender == fWidthStepper || sender == fRatioCheck ||
               sender == fCropTopStepper || sender == fCropBottomStepper||
               sender == fCropMatrix || sender == nil  )
            {
                hb_fix_aspect( job, HB_KEEP_WIDTH );
                if( job->height > fTitle->height )
                {
                    job->height = fTitle->height;
                    hb_fix_aspect( job, HB_KEEP_HEIGHT );
                }
            }
            else
            {
                hb_fix_aspect( job, HB_KEEP_HEIGHT );
                if( job->width > fTitle->width )
                {
                    job->width = fTitle->width;
                    hb_fix_aspect( job, HB_KEEP_WIDTH );
                }
            }

        }
    }

    // hb_get_preview can't handle sizes that are larger than the original title
    if ([fAnamorphicPopUp indexOfSelectedItem] != 3)
    {
        // dimensions
        if( job->width > fTitle->width )
        {
            job->width = fTitle->width;
        }

        if( job->height > fTitle->height )
        {
            job->height = fTitle->height;
        }
    }

    [fWidthStepper      setIntValue: job->width];
    [fWidthField        setIntValue: job->width];
    if( [fAnamorphicPopUp indexOfSelectedItem] != 2) // if we are not loose or custom
    {
        [fHeightStepper     setIntValue: job->height];
        [fHeightField       setIntValue: job->height];
    }

    [fCropTopStepper    setIntValue: job->crop[0]];
    [fCropTopField      setIntValue: job->crop[0]];
    [fCropBottomStepper setIntValue: job->crop[1]];
    [fCropBottomField   setIntValue: job->crop[1]];
    [fCropLeftStepper   setIntValue: job->crop[2]];
    [fCropLeftField     setIntValue: job->crop[2]];
    [fCropRightStepper  setIntValue: job->crop[3]];
    [fCropRightField    setIntValue: job->crop[3]];

    /*
     * Sanity-check here for previews < 16 pixels to avoid crashing
     * hb_get_preview(). In fact, let's get previews at least 64 pixels in both
     * dimensions; no human can see any meaningful detail below that.
     */
    if (job->width >= 64 && job->height >= 64)
    {
        [self reloadStillPreview];
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

- (IBAction) modeDecombDeinterlaceSliderChanged: (id) sender
{
    /* since its a tickless slider, we have to  make sure we are on or off */
    if ([fDecombDeinterlaceSlider floatValue] < 0.50)
    {
        [fDecombDeinterlaceSlider setFloatValue:0.0];
    }
    else
    {
        [fDecombDeinterlaceSlider setFloatValue:1.0];
    }

    /* Decomb selected*/
    if ([fDecombDeinterlaceSlider floatValue] == 0.0)
    {
        [fDecombBox setHidden:NO];
        [fDeinterlaceBox setHidden:YES];
        self.decomb = [fDecombPopUp indexOfSelectedItem];
        _useDecomb = 1;
        self.deinterlace = 0;
        [fDecombPopUp selectItemAtIndex:self.decomb];
        [self adjustFilterDisplay:fDecombPopUp];
    }
    else
    {
        [fDecombBox setHidden:YES];
        [fDeinterlaceBox setHidden:NO];
        _useDecomb = 0;
        self.decomb = 0;
        [fDeinterlacePopUp selectItemAtIndex: self.deinterlace];
        [self adjustFilterDisplay:fDeinterlacePopUp];
    }

    [self FilterSettingsChanged: fDecombDeinterlaceSlider];
}


- (IBAction) FilterSettingsChanged: (id) sender
{
    self.detelecine  = [fDetelecinePopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDetelecinePopUp];

    self.decomb = [fDecombPopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDecombPopUp];

    self.deinterlace = [fDeinterlacePopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDeinterlacePopUp];

    self.denoise = [fDenoisePopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDenoisePopUp];

    if ([[fDeblockField stringValue] isEqualToString:@"Off"])
    {
        self.deblock  = 0;
    }
    else
    {
        self.deblock  = [fDeblockField intValue];
    }

    // Tell PreviewController whether it should deinterlace
    // the previews or not
    if ((self.deinterlace && !self.useDecomb) ||
        (self.decomb && self.useDecomb))
    {
        fPreviewController.deinterlacePreview = YES;
    }
    else
    {
        fPreviewController.deinterlacePreview = NO;
    }
    
    self.grayscale = [fGrayscaleCheck state];
    
    
    if (sender != nil)
    {
        [self.delegate pictureSettingsDidChange];
        [self reloadStillPreview];
    }
}

- (IBAction) deblockSliderChanged: (id) sender
{
    if ([fDeblockSlider floatValue] == 4.0)
    {
        [fDeblockField setStringValue: @"Off"];
    }
    else
    {
        [fDeblockField setStringValue: [NSString stringWithFormat: @"%.0f", [fDeblockSlider floatValue]]];
    }
	[self FilterSettingsChanged: sender];
}

#pragma mark -

- (void) setUseDecomb: (NSInteger) setting
{
    _useDecomb = setting;
    if (self.useDecomb == 1)
    {
        [fDecombDeinterlaceSlider setFloatValue:0.0];
    }
    else
    {
        [fDecombDeinterlaceSlider setFloatValue:1.0];
    }
    
    [self modeDecombDeinterlaceSliderChanged:nil];
}

@end
