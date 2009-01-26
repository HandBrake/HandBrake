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
    //[fPreviewController   setHBController: controller];
    
}

- (void)awakeFromNib
{
    [fFilterWindow setDelegate:self];
    [self setInitialPictureFilters];
}


- (void)windowWillClose:(NSNotification *)aNotification
{

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
	/* we use a popup to show the deinterlace settings */
	[fDeinterlacePopUp removeAllItems];
    [fDeinterlacePopUp addItemWithTitle: @"None"];
    [fDeinterlacePopUp addItemWithTitle: @"Fast"];
    [fDeinterlacePopUp addItemWithTitle: @"Slow"];
	[fDeinterlacePopUp addItemWithTitle: @"Slower"];
    
	/* Set deinterlaces level according to the integer in the main window */
	[fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];

	/* we use a popup to show the denoise settings */
	[fDenoisePopUp removeAllItems];
    [fDenoisePopUp addItemWithTitle: @"None"];
    [fDenoisePopUp addItemWithTitle: @"Weak"];
	[fDenoisePopUp addItemWithTitle: @"Medium"];
    [fDenoisePopUp addItemWithTitle: @"Strong"];
	/* Set denoises level according to the integer in the main window */
	[fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
    

}

- (void) SetTitle: (hb_title_t *) title
{
    /* Set filters widgets according to the filters struct */
	[fDetelecineCheck setState:fPictureFilterSettings.detelecine];
    [fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];
    [fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
    [fDeblockCheck setState: fPictureFilterSettings.deblock];


    
    [self FilterSettingsChanged: nil];
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

- (IBAction) FilterSettingsChanged: (id) sender
{
    fPictureFilterSettings.deinterlace = [fDeinterlacePopUp indexOfSelectedItem];
    
    fPictureFilterSettings.denoise     = [fDenoisePopUp indexOfSelectedItem];
    
    fPictureFilterSettings.detelecine  = [fDetelecineCheck state];
    
    if ([fDeblockField stringValue] == @"Off")
    {
        fPictureFilterSettings.deblock  = 0;
    }
    else
    {
        fPictureFilterSettings.deblock  = [fDeblockField intValue];
    }
    
    fPictureFilterSettings.decomb = [fDecombCheck state];
    
    if (sender != nil)
    {
        [fHBController pictureSettingsDidChange];
        
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

- (int) detelecine
{
    return fPictureFilterSettings.detelecine;
}

- (void) setDetelecine: (int) setting
{
    fPictureFilterSettings.detelecine = setting;
}

- (int) deinterlace
{
    return fPictureFilterSettings.deinterlace;
}

- (void) setDeinterlace: (int) setting {
    fPictureFilterSettings.deinterlace = setting;
}
- (int) decomb
{
    return fPictureFilterSettings.decomb;
}

- (void) setDecomb: (int) setting {
    fPictureFilterSettings.decomb = setting;
}
- (int) denoise
{
    return fPictureFilterSettings.denoise;
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

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title
{
    [self SetTitle:title];
    [self showWindow:sender];

}

@end

