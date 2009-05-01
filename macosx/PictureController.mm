/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "PictureController.h"
#import "Controller.h"
#import "HBPreviewController.h"



@implementation PictureController

- (id)init
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
        
	fPreviewController = [[PreviewController alloc] init];
    }
	return self;
}

//------------------------------------------------------------------------------------
// Displays and brings the picture window to the front
//------------------------------------------------------------------------------------
- (IBAction) showPictureWindow: (id)sender
{
    if ([fPreviewController fullScreen] == YES)
    {
        [self showWindow:sender];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PictureSizeWindowIsOpen"];
        [self setToFullScreenMode];
    }
    else
    {
        if ([[self window] isVisible])
        {
            [[self window] close];
        }
        else
        {
            [self showWindow:sender];
            [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PictureSizeWindowIsOpen"];
            [self setToWindowedMode];
        }
    }
    [self adjustFilterDisplay:nil];
    [self adjustSizingDisplay:nil];
}

- (BOOL) previewFullScreenMode
{
    return [fPreviewController fullScreen];
}

/* this method is used to detect clicking on a tab in fSizeFilterView */
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{

[self resizeInspectorForTab:nil];

}

#pragma mark -

/* resizeInspectorForTab is called at launch, and each time either the 
 * Size or Filters tab is clicked. Size gives a horizontally oriented
 * inspector and Filters is a vertically aligned inspector.
 */
- (IBAction) resizeInspectorForTab: (id)sender
{
    NSRect frame = [[self window] frame];
    NSPoint windowOrigin = [[self window] frame].origin;
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
        frame.size.width = 50.0 + [fPictureSizeBox frame].size.width + [fPictureCropBox frame].size.width;
        frame.size.height = [fPictureSizeBox frame].size.height + 85;
        /* hide the size summary field at the bottom */
        [fSizeInfoField setHidden:NO];      
    }
    /* get delta's for the change in window size */
    CGFloat deltaX = frame.size.width - [[self window] frame].size.width;
    CGFloat deltaY = frame.size.height - [[self window] frame].size.height;
    
    /* Check to see if we have changed the height from current */
    //if (frame.size.height != [[self window] frame].size.height)
    //{
        /* change the inspector origin via the deltaY */
        frame.origin.y -= deltaY;
        /* keep the inspector centered so the tabs stay in place */
        frame.origin.x -= deltaX / 2.0;
    //}
    
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

- (IBAction) adjustSizingDisplay: (id) sender
{
    NSSize pictureSizingBoxSize = [fPictureSizeBox frame].size;
    
    NSPoint fPictureSizeBoxOrigin = [fPictureSizeBox frame].origin;
    NSSize pictureCropBoxSize = [fPictureCropBox frame].size;
    NSPoint fPictureCropBoxOrigin = [fPictureCropBox frame].origin;
    
    if ([fAnamorphicPopUp indexOfSelectedItem] == 3) // custom / power user jamboree
    {
        pictureSizingBoxSize.width = 530;
        
        /* Set visibility of capuj widgets */
        [fParWidthField setHidden: NO];
        [fParHeightField setHidden: NO];
        [fParWidthLabel setHidden: NO];
        [fParHeightLabel setHidden: NO];
        [fDisplayWidthField setHidden: NO];
        [fDisplayWidthLabel setHidden: NO];
        [fModulusLabel setHidden: NO];
        [fModulusPopUp setHidden: NO];
        /* adjust/move keep ar checkbox */
        [fRatioLabel setHidden: YES];
        [fRatioLabel2 setHidden: NO];
        
        /* Optionally swith the Storage and Display width positions*/
         /*
         NSPoint fWidthLabelOrigin = [fWidthLabel frame].origin;
         NSPoint fWidthFieldOrigin = [fWidthField frame].origin;
         NSPoint fWidthStepperOrigin = [fWidthStepper frame].origin;
         fWidthFieldOrigin.x = [fRatioLabel2 frame].origin.x + [fRatioLabel2 frame].size.width + 4;
         [fWidthField setFrameOrigin:fWidthFieldOrigin];

         fWidthStepperOrigin.x = [fWidthField frame].origin.x + [fWidthField frame].size.width + 4;
         [fWidthStepper setFrameOrigin:fWidthStepperOrigin];
         
         fWidthLabelOrigin.x = [fWidthField frame].origin.x - [fWidthLabel frame].size.width - 4;
         [fWidthLabel setFrameOrigin:fWidthLabelOrigin];
         [fWidthLabel setStringValue:@"Storage Width:"];
         */
        
        /* set the origin for fRatioCheck so origin.y == fRatioLabel2
         * and origin.x == fDisplayWidthField
         */
         NSPoint fRatioCheckOrigin = [fRatioCheck frame].origin;
         fRatioCheckOrigin.y = [fRatioLabel2 frame].origin.y - 2;
         fRatioCheckOrigin.x = [fRatioLabel2 frame].origin.x + [fRatioLabel2 frame].size.width + 4;
         [fRatioCheck setFrameOrigin:fRatioCheckOrigin];
         
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
        [fModulusLabel setHidden: YES];
        [fModulusPopUp setHidden: YES];
        /* adjust/move keep ar checkbox */
        [fRatioLabel setHidden: NO];
        [fRatioLabel2 setHidden: YES];
        
         /* Optionally swith the Storage and Display width positions*/
         
         /*
         NSPoint fWidthLabelOrigin = [fWidthLabel frame].origin;
         NSPoint fWidthFieldOrigin = [fWidthField frame].origin;
         NSPoint fWidthStepperOrigin = [fWidthStepper frame].origin;
         
         fWidthFieldOrigin.x = [fHeightField frame].origin.x;
         [fWidthField setFrameOrigin:fWidthFieldOrigin];
         
         fWidthStepperOrigin.x = [fHeightStepper frame].origin.x;
         [fWidthStepper setFrameOrigin:fWidthStepperOrigin];
         
         fWidthLabelOrigin.x = [fWidthField frame].origin.x - [fWidthLabel frame].size.width -4;
         [fWidthLabel setFrameOrigin:fWidthLabelOrigin];
         [fWidthLabel setStringValue:@"Width:"];
         */
        
        
        /* set the origin for fRatioCheck so origin.y == fRatioLabel
         * and origin.x == fWidthStepper
         */
         NSPoint fRatioCheckOrigin = [fRatioCheck frame].origin;
         fRatioCheckOrigin.y = [fRatioLabel frame].origin.y - 2;
         fRatioCheckOrigin.x = [fWidthStepper frame].origin.x - 2;
         [fRatioCheck setFrameOrigin:fRatioCheckOrigin];
        
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

- (IBAction) adjustFilterDisplay: (id) sender
{
    
    NSBox * filterBox = nil;
    NSTextField * filterField;
    if (sender == fDetelecinePopUp)
    {
        filterBox = fDetelecineBox;
        filterField = fDetelecineField;
    }
    
    if (sender == fDecombDeinterlaceSlider)
    {
        if ([fDecombDeinterlaceSlider floatValue] == 0.0)
        {
            filterBox = fDecombBox;
            filterField = fDecombField;
        }
        else
        {
            filterBox = fDeinterlaceBox;
            filterField = fDeinterlaceField;
        }
    }
    
    if (sender == fDecombPopUp)
    {
        filterBox = fDecombBox;
        filterField = fDecombField;
    }
    if (sender == fDeinterlacePopUp)
    {
        filterBox = fDeinterlaceBox;
        filterField = fDeinterlaceField;
    }
    
    if (sender == fDenoisePopUp)
    {
        filterBox = fDenoiseBox;
        filterField = fDenoiseField;
    }
    
    NSSize currentSize = [filterBox frame].size;
    NSRect boxFrame = [filterBox frame];
    
    if ([sender titleOfSelectedItem] == @"Custom")
    {
        
        currentSize.height = 60;
        
    }
    else
    {
        currentSize.height = 30;
        
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
        
        /* go ahead and resize the box */
        //[[filterBox animator] setFrameSize:currentSize];
        //[[filterBox animator] setFrameOrigin:origin];
 
    
        if (filterBox == fDecombBox || filterBox == fDeinterlaceBox)
        {
            /* fDecombDeinterlaceBox*/
            NSSize decombDeinterlaceBoxSize = [fDecombDeinterlaceBox frame].size;
            NSPoint decombDeinterlaceBoxOrigin = [fDecombDeinterlaceBox frame].origin;
            
            //decombDeinterlaceBoxSize.height = [filterBox frame].size.height + 50;
            if (sender == fDecombDeinterlaceSlider)
            {
                [fHBController writeToActivityLog: "Sender is deinterlace decomb slider"];
            }
            
            if ([fDeinterlaceBox isHidden] == YES)
            {
                decombDeinterlaceBoxSize.height = [fDecombBox frame].size.height + 50;
                [fHBController writeToActivityLog: "Resize by Decomb box"];
            }
            else
            {
                decombDeinterlaceBoxSize.height = [fDeinterlaceBox frame].size.height + 50;
                [fHBController writeToActivityLog: "Resize by Deinterlace box"];
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


#pragma mark -

- (IBAction) previewGoWindowed: (id)sender
{
    [fPreviewController goWindowedScreen:self];
}

- (IBAction) showPreviewWindow: (id)sender
{
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PreviewWindowIsOpen"];
    [fPreviewController showWindow:sender];
}




- (void) setToFullScreenMode
{
    int32_t shieldLevel = CGShieldingWindowLevel(); 
    
    [fPictureWindow setLevel:shieldLevel + 1]; 
    // Show the window. 
    [fPictureWindow makeKeyAndOrderFront:self];
}

- (void) setToWindowedMode
{
    /* Set the window back to Floating Window mode 
     * This will put the window always on top, but
     * since we have Hide on Deactivate set in our
     * xib, if other apps are put in focus we will
     * hide properly to stay out of the way
     */
    [[self window] setLevel:NSFloatingWindowLevel];
}

- (void)setHBController: (HBController *)controller
{
    fHBController = controller;
    [fPreviewController   setHBController: controller];
    
}

- (void)awakeFromNib
{
    [fPictureWindow setDelegate:self];
    if( ![[self window] setFrameUsingName:@"PictureSizing"] )
        [[self window] center];
    [self setWindowFrameAutosaveName:@"PictureSizing"];
    [[self window] setExcludedFromWindowsMenu:YES];
    
    [self setInitialPictureFilters];
    
    /* Setup our layers for core animation */
    [fSizeFilterView setWantsLayer:YES];
    [fPictureSizeBox setWantsLayer:YES];
    [fPictureCropBox setWantsLayer:YES];
    
}


- (void)windowWillClose:(NSNotification *)aNotification
{
[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PictureSizeWindowIsOpen"];
}

- (BOOL)windowShouldClose:(id)fPictureWindow
{
    return YES;
}

- (void) dealloc
{
    [fPreviewController release];
    [super dealloc];
}

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle = handle;
    
    [fPreviewController SetHandle: fHandle];
}

- (void) SetTitle: (hb_title_t *) title
{
    hb_job_t * job = title->job;

    fTitle = title;

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

    /* Populate the Anamorphic NSPopUp button here */
    [fAnamorphicPopUp removeAllItems];
    [fAnamorphicPopUp addItemWithTitle: @"None"];
    [fAnamorphicPopUp addItemWithTitle: @"Strict"];
    if (allowLooseAnamorphic)
    {
    [fAnamorphicPopUp addItemWithTitle: @"Loose"];
    }
    [fAnamorphicPopUp addItemWithTitle: @"Custom"];
    [fAnamorphicPopUp selectItemAtIndex: job->anamorphic.mode];
    
    //[self adjustSizingDisplay:nil];
    
    /* populate the modulus popup here */
    [fModulusPopUp removeAllItems];
    [fModulusPopUp addItemWithTitle: @"16"];
    [fModulusPopUp addItemWithTitle: @"8"];
    [fModulusPopUp addItemWithTitle: @"4"];
    [fModulusPopUp addItemWithTitle: @"2"];
    if (job->anamorphic.mode == 3)
    {
        [fModulusPopUp selectItemWithTitle: [NSString stringWithFormat:@"%d",job->anamorphic.modulus]];
    }
    else
    {
        [fModulusPopUp selectItemWithTitle: @"16"];
    }
    
    /* We initially set the previous state of keep ar to on */
    keepAspectRatioPreviousState = 1;
	if (!autoCrop)
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
    [fDetelecinePopUp selectItemAtIndex:fPictureFilterSettings.detelecine];
    [fDecombPopUp selectItemAtIndex:fPictureFilterSettings.decomb];
    [fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];
    [fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
    [fDeblockSlider setFloatValue:fPictureFilterSettings.deblock];
    [fGrayscaleCheck setState:fPictureFilterSettings.grayscale];
    
    [self deblockSliderChanged: nil];
    
    fPicture = 0;
    MaxOutputWidth = title->width - job->crop[2] - job->crop[3];
    MaxOutputHeight = title->height - job->crop[0] - job->crop[1];
    
    titleDarWidth = job->anamorphic.dar_width;
    titleDarHeight = job->anamorphic.dar_height;
    
    titleParWidth = job->anamorphic.par_width;
    titleParHeight = job->anamorphic.par_height;
    
    [self SettingsChanged: nil];
}

- (IBAction) storageLinkChanged: (id) sender
{
    /* since we have a tickless slider, make sure we are at 0.0 or 1.0 */
    if ([fStorageLinkSlider floatValue] < 0.50)
    {
        [fStorageLinkSlider setFloatValue:0.0];
        /* set slider labels to reflect choice */
        [fStorageLinkParLabel setEnabled:YES];
        [fStorageLinkDisplayLabel setEnabled:NO];

    }
    else
    {
        [fStorageLinkSlider setFloatValue:1.0];
        /* set slider labels to reflect choice */
        [fStorageLinkParLabel setEnabled:NO];
        [fStorageLinkDisplayLabel setEnabled:YES];
    }
    
}

- (IBAction) parLinkChanged: (id) sender
{
    /* since we have a tickless slider, make sure we are at 0.0 or 1.0 */
    if ([fParLinkSlider floatValue] < 0.50)
    {
        [fParLinkSlider setFloatValue:0.0];
        /* set slider labels to reflect choice */
        [fParLinkStorageLabel setEnabled:YES];
        [fParLinkDisplayLabel setEnabled:NO];
    }
    else
    {
        [fParLinkSlider setFloatValue:1.0];
        /* set slider labels to reflect choice */
        [fParLinkStorageLabel setEnabled:NO];
        [fParLinkDisplayLabel setEnabled:YES];
    }
    
}

- (IBAction) displayLinkChanged: (id) sender
{
    /* since we have a tickless slider, make sure we are at 0.0 or 1.0 */
    if ([fDisplayLinkSlider floatValue] < 0.50)
    {
        [fDisplayLinkSlider setFloatValue:0.0];
        /* set slider labels to reflect choice */
        [fDisplayLinkStorageLabel setEnabled:YES];
        [fDisplayLinkParLabel setEnabled:NO];
    }
    else
    {
        [fDisplayLinkSlider setFloatValue:1.0];
        /* set slider labels to reflect choice */
        [fDisplayLinkStorageLabel setEnabled:NO];
        [fDisplayLinkParLabel setEnabled:YES];
    }
    
}    

- (IBAction) SettingsChanged: (id) sender
{
    hb_job_t * job = fTitle->job;
    [fModulusPopUp setEnabled:NO];
    job->anamorphic.modulus = 16;

    /* Since custom anamorphic allows for a height setting > fTitle->height
     * check to make sure it is returned to fTitle->height for all other modes
     */
     [fHeightStepper setMaxValue: fTitle->height];
    
    autoCrop = ( [fCropMatrix selectedRow] == 0 );
    [fCropTopStepper    setEnabled: !autoCrop];
    [fCropBottomStepper setEnabled: !autoCrop];
    [fCropLeftStepper   setEnabled: !autoCrop];
    [fCropRightStepper  setEnabled: !autoCrop];
    
    if( autoCrop )
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
        
        [fWidthStepper  setIncrement: 16];
        [fHeightStepper setIncrement: 16];
    }
    else
    {
        [fWidthStepper  setIncrement: [[fModulusPopUp titleOfSelectedItem] intValue]];
        [fHeightStepper setIncrement: [[fModulusPopUp titleOfSelectedItem] intValue]];
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

#pragma mark - STARTCapuj

            job->anamorphic.mode = 3;
            
            /* Set the status of our custom ana only widgets accordingly */
            /* for mod 3 we can use modulus other than 16 */
            [fModulusPopUp setEnabled:YES];
            job->anamorphic.modulus = [[fModulusPopUp titleOfSelectedItem] intValue];
            
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
            
            
            /* If we are coming into custom ana or if in custom ana and the 
             * keep ar checkbox is checked, we reset the par to original
             * which gives us a way back if things are hosed up
             */
             
            if (sender == fAnamorphicPopUp || (sender == fRatioCheck && [fRatioCheck  state] == NSOnState))
            {
                if (sender == fAnamorphicPopUp)
                {
                    [fRatioCheck  setState: NSOnState];
                }
                
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
            
#pragma mark - END Capuj                       
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
	
    //job->keep_ratio  = ( [fRatioCheck state] == NSOnState );
    
    if ([fAnamorphicPopUp indexOfSelectedItem] != 3)
    {
    job->keep_ratio  = ( [fRatioCheck state] == NSOnState );
            if( job->keep_ratio )
        {
            if( sender == fWidthStepper || sender == fRatioCheck ||
               sender == fCropTopStepper || sender == fCropBottomStepper )
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
    
    //[fPreviewController SetTitle:fTitle];
    
    /* Sanity Check Here for < 16 px preview to avoid
     crashing hb_get_preview. In fact, just for kicks
     lets getting previews at a min limit of 32, since
     no human can see any meaningful detail below that */
    if (job->width >= 64 && job->height >= 64)
    {
        [self reloadStillPreview];
    }
    
    /* we get the sizing info to display from fPreviewController */
    [fSizeInfoField setStringValue: [fPreviewController pictureSizeInfoString]];
    
    if (sender != nil)
    {
        [fHBController pictureSettingsDidChange];
    }   
    
    if ([[self window] isVisible])
    { 
        [self adjustSizingDisplay:nil];
    }
    
}

- (NSString*) getPictureSizeInfoString
{
    return [fSizeInfoField stringValue];
}

- (void)reloadStillPreview
{ 
   hb_job_t * job = fTitle->job; 
   
   [fPreviewController SetTitle:fTitle];
    /* Sanity Check Here for < 16 px preview to avoid
     crashing hb_get_preview. In fact, just for kicks
     lets getting previews at a min limit of 32, since
     no human can see any meaningful detail below that */
    if (job->width >= 64 && job->height >= 64)
    {
       
         // Purge the existing picture previews so they get recreated the next time
        // they are needed.
      //  [fPreviewController purgeImageCache];
        /* We actually call displayPreview now from pictureSliderChanged which keeps
         * our picture preview slider in sync with the previews being shown
         */

    //[fPreviewController pictureSliderChanged:nil];
    }
    
}


#pragma mark -

- (BOOL) autoCrop
{
    return autoCrop;
}
- (void) setAutoCrop: (BOOL) setting
{
    autoCrop = setting;
}

- (BOOL) allowLooseAnamorphic
{
    return allowLooseAnamorphic;
}

- (void) setAllowLooseAnamorphic: (BOOL) setting
{
    allowLooseAnamorphic = setting;
}

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title
{
    //[self SetTitle:title];
    [self showWindow:sender];
    //[self adjustSizingDisplay:nil];
    //[self adjustFilterDisplay:nil];

}


#pragma mark -
/* we use this to setup the initial picture filters upon first launch, after that their states
are maintained across different sources */
- (void) setInitialPictureFilters
{
	/* we use a popup to show the detelecine settings */
	[fDetelecinePopUp removeAllItems];
    [fDetelecinePopUp addItemWithTitle: @"Off"];
    [fDetelecinePopUp addItemWithTitle: @"Default"];
    [fDetelecinePopUp addItemWithTitle: @"Custom"];
    [fDetelecinePopUp selectItemAtIndex: fPictureFilterSettings.detelecine];
    
    [self modeDecombDeinterlaceSliderChanged:nil];
    /* we use a popup to show the decomb settings */
	[fDecombPopUp removeAllItems];
    [fDecombPopUp addItemWithTitle: @"Off"];
    [fDecombPopUp addItemWithTitle: @"Default"];
    [fDecombPopUp addItemWithTitle: @"Custom"];
    [fDecombPopUp selectItemAtIndex: fPictureFilterSettings.decomb];
    
    /* we use a popup to show the deinterlace settings */
	[fDeinterlacePopUp removeAllItems];
    [fDeinterlacePopUp addItemWithTitle: @"None"];
    [fDeinterlacePopUp addItemWithTitle: @"Fast"];
    [fDeinterlacePopUp addItemWithTitle: @"Slow"];
	[fDeinterlacePopUp addItemWithTitle: @"Slower"];
    [fDeinterlacePopUp addItemWithTitle: @"Custom"];
    
	/* Set deinterlaces level according to the integer in the main window */
	[fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];

	/* we use a popup to show the denoise settings */
	[fDenoisePopUp removeAllItems];
    [fDenoisePopUp addItemWithTitle: @"None"];
    [fDenoisePopUp addItemWithTitle: @"Weak"];
	[fDenoisePopUp addItemWithTitle: @"Medium"];
    [fDenoisePopUp addItemWithTitle: @"Strong"];
    [fDenoisePopUp addItemWithTitle: @"Custom"];
	/* Set denoises level according to the integer in the main window */
	[fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
    

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
        fPictureFilterSettings.decomb = [fDecombPopUp indexOfSelectedItem];
        fPictureFilterSettings.usedecomb = 1;
        fPictureFilterSettings.deinterlace = 0;
        [fDecombPopUp selectItemAtIndex:fPictureFilterSettings.decomb];
        [self adjustFilterDisplay:fDecombPopUp];
    }
    else
    {
        [fDecombBox setHidden:YES];
        [fDeinterlaceBox setHidden:NO];
        fPictureFilterSettings.usedecomb = 0;
        fPictureFilterSettings.decomb = 0;
        [fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];
        [self adjustFilterDisplay:fDeinterlacePopUp];
    }
    [self FilterSettingsChanged: fDecombDeinterlaceSlider];
}


- (IBAction) FilterSettingsChanged: (id) sender
{
    
    fPictureFilterSettings.detelecine  = [fDetelecinePopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDetelecinePopUp];
    
    fPictureFilterSettings.decomb = [fDecombPopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDecombPopUp];
    
    fPictureFilterSettings.deinterlace = [fDeinterlacePopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDeinterlacePopUp];
    
    fPictureFilterSettings.denoise = [fDenoisePopUp indexOfSelectedItem];
    [self adjustFilterDisplay:fDenoisePopUp];
    
    if ([fDeblockField stringValue] == @"Off")
    {
        fPictureFilterSettings.deblock  = 0;
    }
    else
    {
        fPictureFilterSettings.deblock  = [fDeblockField intValue];
    }
    
    fPictureFilterSettings.grayscale = [fGrayscaleCheck state];
    
    if (sender != nil)
    {
        [fHBController pictureSettingsDidChange];
        
    }   

}


#pragma mark -

- (IBAction) deblockSliderChanged: (id) sender
{
    if ([fDeblockSlider floatValue] == 4.0)
    {
    [fDeblockField setStringValue: [NSString stringWithFormat: @"Off"]];
    }
    else
    {
    [fDeblockField setStringValue: [NSString stringWithFormat: @"%.0f", [fDeblockSlider floatValue]]];
    }
	[self FilterSettingsChanged: sender];
}


- (int) detelecine
{
    return fPictureFilterSettings.detelecine;
}

- (NSString*) detelecineCustomString
{
    return [fDetelecineField stringValue];
}

- (void) setDetelecine: (int) setting
{
    fPictureFilterSettings.detelecine = setting;
}

- (void) setDetelecineCustomString: (NSString*) string 
{
    [fDetelecineField setStringValue:string];
}

- (int) deinterlace
{
    return fPictureFilterSettings.deinterlace;
}
- (NSString*) deinterlaceCustomString
{
    return [fDeinterlaceField stringValue];
}

- (void) setDeinterlaceCustomString: (NSString*) string 
{
    [fDeinterlaceField setStringValue:string];
}

- (void) setDeinterlace: (int) setting 
{
    fPictureFilterSettings.deinterlace = setting;
}
- (int) decomb
{
    return fPictureFilterSettings.decomb;
}

- (NSString*) decombCustomString
{
    return [fDecombField stringValue];
}

- (int) useDecomb
{
    return fPictureFilterSettings.usedecomb;
}

- (void) setUseDecomb: (int) setting
{
    fPictureFilterSettings.usedecomb = setting;
    if (fPictureFilterSettings.usedecomb == 1)
    {
        [fDecombDeinterlaceSlider setFloatValue:0.0];
    }
    else
    {
        [fDecombDeinterlaceSlider setFloatValue:1.0];
    }
    [self modeDecombDeinterlaceSliderChanged:nil];
}

- (void) setDecomb: (int) setting {
    fPictureFilterSettings.decomb = setting;
}

- (void) setDecombCustomString: (NSString*) string 
{
    [fDecombField setStringValue:string];
}

- (int) denoise
{
    return fPictureFilterSettings.denoise;
}

- (NSString*) denoiseCustomString
{
    return [fDenoiseField stringValue];
}

- (void) setDenoiseCustomString: (NSString*) string 
{
    [fDenoiseField setStringValue:string];
}

- (void) setDenoise: (int) setting
{
    fPictureFilterSettings.denoise = setting;
}

- (int) deblock
{
    return fPictureFilterSettings.deblock;
}

- (void) setDeblock: (int) setting
{
    fPictureFilterSettings.deblock = setting;
}

- (int) grayscale
{
    return fPictureFilterSettings.grayscale;
}

- (void) setGrayscale: (int) setting
{
    fPictureFilterSettings.grayscale = setting;
}



@end

