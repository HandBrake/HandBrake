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
}

- (BOOL) previewFullScreenMode
{
    return [fPreviewController fullScreen];
}

- (IBAction) previewGoWindowed: (id)sender
{
    [fPreviewController goWindowedScreen:self];
}

- (IBAction) showPreviewWindow: (id)sender
{
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
    
    [fPreviewController SetHandle: fHandle];
}

- (void) SetTitle: (hb_title_t *) title
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

    /* Populate the Anamorphic NSPopUp button here */
    [fAnamorphicPopUp removeAllItems];
    [fAnamorphicPopUp addItemWithTitle: @"None"];
    [fAnamorphicPopUp addItemWithTitle: @"Strict"];
    if (allowLooseAnamorphic)
    {
    [fAnamorphicPopUp addItemWithTitle: @"Loose"];
    }
    [fAnamorphicPopUp selectItemAtIndex: job->anamorphic.mode];
    
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
    
    [self SettingsChanged: nil];
}

    

- (IBAction) SettingsChanged: (id) sender
{
    hb_job_t * job = fTitle->job;
    
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
    
	if( [fAnamorphicPopUp indexOfSelectedItem] > 0 )
	{
        if ([fAnamorphicPopUp indexOfSelectedItem] == 2) // Loose anamorphic
        {
            job->anamorphic.mode = 2;
            [fWidthStepper setEnabled: YES];
            [fWidthField setEnabled: YES];
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
        else // must be "1" or strict anamorphic
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
        }
        
        /* if the sender is the Anamorphic checkbox, record the state
         of KeepAspect Ratio so it can be reset if Anamorphic is unchecked again */
        if (sender == fAnamorphicPopUp)
        {
            keepAspectRatioPreviousState = [fRatioCheck state];
        }
        [fRatioCheck setState:NSOffState];
        [fRatioCheck setEnabled: NO];
        
        
        [fHeightStepper setEnabled: NO];
        [fHeightField setEnabled: NO];
        
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
        // hb_get_preview can't handle sizes that are larger than the original title
        // dimensions
        if( job->width > fTitle->width )
            job->width = fTitle->width;

        if( job->height > fTitle->height )
            job->height = fTitle->height;
    }

    [fWidthStepper      setIntValue: job->width];
    [fWidthField        setIntValue: job->width];
    if( [fAnamorphicPopUp indexOfSelectedItem] < 2 )
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
    
    [fPreviewController SetTitle:fTitle];
    /* Sanity Check Here for < 16 px preview to avoid
     crashing hb_get_preview. In fact, just for kicks
     lets getting previews at a min limit of 32, since
     no human can see any meaningful detail below that */
    if (job->width >= 64 && job->height >= 64)
    {
       
         // Purge the existing picture previews so they get recreated the next time
        // they are needed.
        [fPreviewController purgeImageCache];
        /* We actually call displayPreview now from pictureSliderChanged which keeps
         * our picture preview slider in sync with the previews being shown
         */

    //[fPreviewController pictureSliderChanged:nil];
    [self reloadStillPreview];
    }

    /* we get the sizing info to display from fPreviewController */
    [fSizeInfoField setStringValue: [fPreviewController pictureSizeInfoString]];

    if (sender != nil)
    {
        [fHBController pictureSettingsDidChange];
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
        [fPreviewController purgeImageCache];
        /* We actually call displayPreview now from pictureSliderChanged which keeps
         * our picture preview slider in sync with the previews being shown
         */

    [fPreviewController pictureSliderChanged:nil];
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
    [self SetTitle:title];
    [self showWindow:sender];

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
    /* Decomb selected*/
    if ([fDecombDeinterlaceSlider floatValue] == 0.0)
    {
    [fDecombBox setHidden:NO];
    [fDeinterlaceBox setHidden:YES];
    fPictureFilterSettings.decomb = [fDecombPopUp indexOfSelectedItem];
    fPictureFilterSettings.usedecomb = 1;
    fPictureFilterSettings.deinterlace = 0;
    [self adjustFilterDisplay:fDecombPopUp];
    [fDecombPopUp selectItemAtIndex:fPictureFilterSettings.decomb];
    }
    else
    {
    [fDecombBox setHidden:YES];
    [fDeinterlaceBox setHidden:NO];
    fPictureFilterSettings.usedecomb = 0;
    fPictureFilterSettings.decomb = 0;
    [self adjustFilterDisplay:fDeinterlacePopUp];
    [fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];
    }
	[self FilterSettingsChanged: sender];
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

- (void) adjustFilterDisplay: (id) sender
{
    
    NSBox * filterBox = nil;
    NSTextField * filterField;
    if (sender == fDetelecinePopUp)
    {
        filterBox = fDetelecineBox;
        filterField = fDetelecineField;
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
        NSPoint origin = [filterBox frame].origin;
        /* See if we are expanding the box downwards */
        if (currentSize.height > [filterBox frame].size.height)
        {
            origin.y = origin.y - currentSize.height / 2;
        }
        else
        {
            origin.y = origin.y + currentSize.height;
        }
        /* go ahead and resize the box */
        [filterBox setFrameSize:currentSize];
        [filterBox setFrameOrigin:origin];
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

