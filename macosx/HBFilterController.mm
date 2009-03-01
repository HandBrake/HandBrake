/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBFilterController.h"
#import "PictureController.h"
#import "Controller.h"
#import "HBPreviewController.h"



@implementation PictureFilterController

- (id)init
{
	if (self = [super initWithWindowNibName:@"PictureFilters"])
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
- (IBAction) showFilterWindow: (id)sender
{
    
    if ([[self window] isVisible])
    {
        [[self window] close];
    }
    else
    {
        [self showWindow:sender];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PictureFiltersWindowIsOpen"];
        if ([fPreviewController fullScreen] == YES)
        {
            [self setToFullScreenMode];
        }
        else
        {
            [self setToWindowedMode];
        }
    }
}

- (IBAction) showPictureSettingsWindow: (id)sender
{
    [fHBController showPicturePanel:sender];
}

- (IBAction) showPreviewWindow: (id)sender
{
    [fHBController showPreviewWindow:sender];
}

- (void) setToFullScreenMode
{
    int32_t shieldLevel = CGShieldingWindowLevel(); 
    
    [fFilterWindow setLevel:shieldLevel + 1]; 
    // Show the window. 
    [fFilterWindow makeKeyAndOrderFront:self];
}

- (void) setToWindowedMode
{
    /* Set the window back to regular level */
    [[self window] setLevel:NSNormalWindowLevel];
}

- (void)setHBController: (HBController *)controller
{
    
    fHBController = controller;
    
}

- (void)awakeFromNib
{
    [fFilterWindow setDelegate:self];
    
    if( ![[self window] setFrameUsingName:@"PictureFilters"] )
        [[self window] center];
    [self setWindowFrameAutosaveName:@"PictureFilters"];
    [[self window] setExcludedFromWindowsMenu:YES];
    
    [self setInitialPictureFilters];

}


- (void)windowWillClose:(NSNotification *)aNotification
{
[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PictureFiltersWindowIsOpen"];
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
    

    
    //[fPreviewController SetHandle: fHandle];
    

}

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

- (void) SetTitle: (hb_title_t *) title
{
    /* Set filters widgets according to the filters struct */
    [fDetelecinePopUp selectItemAtIndex:fPictureFilterSettings.detelecine];
    [fDecombPopUp selectItemAtIndex:fPictureFilterSettings.decomb];
    [fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];
    [fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
    [fDeblockSlider setFloatValue:fPictureFilterSettings.deblock];
    [fGrayscaleCheck setState:fPictureFilterSettings.grayscale];
    
    [self deblockSliderChanged: nil];
}

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

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title
{
    [self SetTitle:title];
    [self showWindow:sender];

}

@end

