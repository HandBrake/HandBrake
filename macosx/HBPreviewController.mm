/* $Id: HBPreviewController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBPreviewController.h"
#import "Controller.h"

@implementation QTMovieView ( HBQTkitExt )
- (void) mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];
}
@end

@interface PreviewController (Private)

- (NSSize)optimalViewSizeForImageSize: (NSSize)imageSize;
- (void)resizeSheetForViewSize: (NSSize)viewSize;
- (void)setViewSize: (NSSize)viewSize;
- (BOOL)viewNeedsToResizeToSize: (NSSize)newSize;

@end

@implementation PreviewController

- (id)init
{
	if (self = [super initWithWindowNibName:@"PicturePreview"])
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
        
		fPicturePreviews = [[NSMutableDictionary dictionaryWithCapacity: HB_NUM_HBLIB_PICTURES] retain];
        /* Init libhb with check for updates libhb style set to "0" so its ignored and lets sparkle take care of it */
        int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
        fPreviewLibhb = hb_init(loggingLevel, 0);
        
	}
	return self;
}



//------------------------------------------------------------------------------------
// Displays and brings the picture window to the front
//------------------------------------------------------------------------------------
- (IBAction) showPreviewWindow: (id)sender
{
    [self showWindow:sender];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PreviewWindowIsOpen"];
    
    /* lets set the preview window to accept mouse moved events */
    [fPreviewWindow setAcceptsMouseMovedEvents:YES];
    hudTimerSeconds = 0;
    [self pictureSliderChanged:nil];
    [self startReceivingLibhbNotifications];
}

- (void)setHBController: (HBController *)controller
{
    fHBController = controller;
}

- (void)awakeFromNib
{
    [fPreviewWindow setDelegate:self];
    if( ![[self window] setFrameUsingName:@"Preview"] )
        [[self window] center];
    [self setWindowFrameAutosaveName:@"Preview"];
    [[self window] setExcludedFromWindowsMenu:YES];
    
    /* lets set the preview window to accept mouse moved events */
    [fPreviewWindow setAcceptsMouseMovedEvents:YES];
    //[self pictureSliderChanged:nil];
    [self startReceivingLibhbNotifications];
    
    isFullScreen = NO;
    hudTimerSeconds = 0;
    /* we set the progress indicator to not use threaded animation
     * as it causes a conflict with the qtmovieview's controllerbar
    */
    [fMovieCreationProgressIndicator setUsesThreadedAnimation:NO];
    
    /* Setup our layers for core animation */
    [fPictureViewArea setWantsLayer:YES];
    [fPictureView setWantsLayer:YES];

    [fCancelPreviewMovieButton setWantsLayer:YES];
    [fMovieCreationProgressIndicator setWantsLayer:YES];

    [fPictureControlBox setWantsLayer:YES];
    [fEncodingControlBox setWantsLayer:YES];
	[fMovieView setWantsLayer:YES];
	[fMovieView setHidden:YES];
    [fMovieView setDelegate:self];

    /* Since the xib has everything off center for easy acess
     * we align our views and windows here we an align to anything
     * since it will actually change later upon source load, but
     * for convenience we will use the fPictureViewArea
     */
     
     /* Align the still preview image view to the picture box */
     [fPictureView setFrameSize:[fPictureViewArea frame].size];
     [fMovieView setFrameSize:[fPictureViewArea frame].size];
     //[fPreviewWindow setFrameSize:[fPictureViewArea frame].size];
    
    
}
- (BOOL)acceptsMouseMovedEvents
{
    return YES;
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    /* Upon Closing the picture window, we make sure we clean up any
     * preview movie that might be playing
     */
    hb_stop( fPreviewLibhb );
    isEncoding = NO;
    // Show the picture view
    [fPictureView setHidden:NO];
    [fMovieView pause:nil];
    [fMovieView setHidden:YES];
	[fMovieView setMovie:nil];

    isFullScreen = NO;
    hudTimerSeconds = 0;
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PreviewWindowIsOpen"];
}

- (BOOL)windowShouldClose:(id)fPictureWindow
{
     return YES;
}

- (void) dealloc
{
    hb_stop(fPreviewLibhb);
    if (fPreviewMoviePath)
    {
        [[NSFileManager defaultManager] removeFileAtPath:fPreviewMoviePath handler:nil];
        [fPreviewMoviePath release];
    }    
    
    [fLibhbTimer invalidate];
    [fLibhbTimer release];
    
    [fHudTimer invalidate];
    [fHudTimer release];
    
    [fPicturePreviews release];
    [fFullScreenWindow release];

    [super dealloc];
}

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle = handle;
    

    
    /* we set the preview length popup in seconds */
    [fPreviewMovieLengthPopUp removeAllItems];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"5"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"10"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"15"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"20"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"25"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"30"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"35"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"40"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"45"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"50"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"55"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"60"];
    
    /* adjust the preview slider length */
    /* We use our advance pref to determine how many previews we scanned */
    int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
    [fPictureSlider setMaxValue: hb_num_previews - 1.0];
    [fPictureSlider setNumberOfTickMarks: hb_num_previews];
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewLength"])
    {
        [fPreviewMovieLengthPopUp selectItemWithTitle:[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewLength"]];
    }
    else
    {
        /* currently hard set default to 10 seconds */
        [fPreviewMovieLengthPopUp selectItemAtIndex: 1];
    }
}

- (void) SetTitle: (hb_title_t *) title
{
    hb_job_t * job = title->job;
    
    fTitle = title;
    fPicture = 0;
    MaxOutputWidth = title->width - job->crop[2] - job->crop[3];
    MaxOutputHeight = title->height - job->crop[0] - job->crop[1];
    [self SettingsChanged: nil];
}



// Adjusts the window to draw the current picture (fPicture) adjusting its size as
// necessary to display as much of the picture as possible.
- (void) displayPreview
{
    hb_job_t * job = fTitle->job;
    /* lets make sure that the still picture view is not hidden and that 
     * the movie preview is 
     */
    [fMovieView pause:nil];
    [fMovieView setHidden:YES];
	[fMovieView setMovie:nil];
    [fMovieCreationProgressIndicator stopAnimation: nil];
    [fMovieCreationProgressIndicator setHidden: YES];
    
    [fPictureView setHidden:NO];
    
    //[fHBController writeToActivityLog: "displayPreview called"];
    
    NSImage *fPreviewImage = [self imageForPicture: fPicture];
    NSSize imageScaledSize = [fPreviewImage size];
    [fPictureView setImage: fPreviewImage];
    
    NSSize displaySize = NSMakeSize( ( CGFloat )fTitle->width, ( CGFloat )fTitle->height );
    NSString *sizeInfoString;
    /* Set the picture size display fields below the Preview Picture*/
    if( fTitle->job->anamorphic.mode == 1 ) // Original PAR Implementation
    {
        output_width = fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3];
        output_height = fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1];
        display_width = output_width * fTitle->job->anamorphic.par_width / fTitle->job->anamorphic.par_height;
        sizeInfoString = [NSString stringWithFormat:
                          @"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d Strict",
                          fTitle->width, fTitle->height, output_width, output_height, display_width, output_height];
        
        displaySize.width = display_width;
        displaySize.height = fTitle->height;
        imageScaledSize.width = display_width;
        imageScaledSize.height = output_height;   
    }
    else if (fTitle->job->anamorphic.mode == 2) // Loose Anamorphic
    {
        hb_set_anamorphic_size(job, &output_width, &output_height, &output_par_width, &output_par_height);
        display_width = output_width * output_par_width / output_par_height;
        sizeInfoString = [NSString stringWithFormat:
                          @"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d Loose",
                          fTitle->width, fTitle->height, output_width, output_height, display_width, output_height];
        
        displaySize.width = display_width;
        displaySize.height = fTitle->height;
        imageScaledSize.width = display_width;
        imageScaledSize.height = output_height;
    }
    else if (fTitle->job->anamorphic.mode == 3) // Custom Anamorphic
    {
        hb_set_anamorphic_size(job, &output_width, &output_height, &output_par_width, &output_par_height);
        display_width = output_width * output_par_width / output_par_height;
        sizeInfoString = [NSString stringWithFormat:
                          @"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d Custom",
                          fTitle->width, fTitle->height, output_width, output_height, fTitle->job->anamorphic.dar_width, fTitle->job->anamorphic.dar_height];
        
        displaySize.width = fTitle->job->anamorphic.dar_width + fTitle->job->crop[2] + fTitle->job->crop[3];
        displaySize.height = fTitle->job->anamorphic.dar_height + fTitle->job->crop[0] + fTitle->job->crop[1];
        imageScaledSize.width = (int)fTitle->job->anamorphic.dar_width;
        imageScaledSize.height = (int)fTitle->job->height;   
    } 
    else // No Anamorphic
    {
        sizeInfoString = [NSString stringWithFormat:
                          @"Source: %dx%d, Output: %dx%d", fTitle->width, fTitle->height,
                          fTitle->job->width, fTitle->job->height];
        
        displaySize.width = fTitle->width;
        displaySize.height = fTitle->height;
        imageScaledSize.width = fTitle->job->width;
        imageScaledSize.height = fTitle->job->height;
    }
    
    NSSize viewSize = [self optimalViewSizeForImageSize:displaySize];
    
    /* Initially set our preview image here */
    /*
    if (scaleToScreen == YES)
    {
        viewSize.width = viewSize.width - (viewSize.width - imageScaledSize.width);
        viewSize.height = viewSize.height - (viewSize.height - imageScaledSize.height);
        [fPreviewImage setSize: viewSize];
        //[fPictureView setFrameSize: viewSize];
    }
    
    else
    {
        [fPreviewImage setSize: imageScaledSize];
        [fPictureView setFrameSize: imageScaledSize];
    }
    [fPictureView setImage: fPreviewImage];
    // center it vertically and horizontally
    NSPoint origin = [fPictureViewArea frame].origin;
    origin.y += ([fPictureViewArea frame].size.height -
                 [fPictureView frame].size.height) / 2.0;
    
    origin.x += ([fPictureViewArea frame].size.width -
                 [fPictureView frame].size.width) / 2.0;
    [fPictureView setFrameOrigin:origin]; 
    */
    /* we also need to take into account scaling to full screen to activate switching the view size */
    if( [self viewNeedsToResizeToSize:viewSize])
    {
        if (fTitle->job->anamorphic.mode != 2 || (fTitle->job->anamorphic.mode == 2 && fTitle->width == fTitle->job->width))
        {
            [self resizeSheetForViewSize:viewSize];
            //[self setViewSize:viewSize];
            
        }
    }   
    
    viewSize.width = viewSize.width - (viewSize.width - imageScaledSize.width);
    viewSize.height = viewSize.height - (viewSize.height - imageScaledSize.height);
    [self setViewSize:viewSize];
    
    /* special case for scaleToScreen */
    if (scaleToScreen == YES)
    {
        [fPreviewImage setSize: viewSize];
        [fPictureView setImage: fPreviewImage];
    }
    
    NSString *scaleString;
    
    if( imageScaledSize.height > [fPictureView frame].size.height)
    {
        CGFloat scale = ( ( CGFloat )[fPictureView frame].size.width) / ( ( CGFloat )imageScaledSize.width);        
        scaleString = [NSString stringWithFormat:
                       NSLocalizedString( @" (Scaled to %.0f%% actual size)",
                                         @"String shown when a preview is scaled" ), scale * 100.0];
    }
    else
    {
        scaleString = @"";
    }
    /* Set the info fields in the hud controller */
    [fInfoField setStringValue: [NSString stringWithFormat:
                                 @"%@", sizeInfoString]];
    
    [fscaleInfoField setStringValue: [NSString stringWithFormat:
                                      @"%@", scaleString]];
    /* Set the info field in the window title bar */
    [[self window] setTitle:[NSString stringWithFormat: @"Preview - %@ %@",sizeInfoString, scaleString]];
}

- (IBAction) previewDurationPopUpChanged: (id) sender
{
    
    [[NSUserDefaults standardUserDefaults] setObject:[fPreviewMovieLengthPopUp titleOfSelectedItem] forKey:@"PreviewLength"];
    
}    
    
- (IBAction) SettingsChanged: (id) sender
{
         // Purge the existing picture previews so they get recreated the next time
        // they are needed.
        [self purgeImageCache];
        [self pictureSliderChanged:nil];
}

- (IBAction) pictureSliderChanged: (id) sender
{
    // Show the picture view
    [fPictureView setHidden:NO];
    [fMovieView pause:nil];
    [fMovieView setHidden:YES];
	[fMovieView setMovie:nil];
    [fEncodingControlBox setHidden: YES];
    
    int newPicture = [fPictureSlider intValue];
    if (newPicture != fPicture)
    {
        fPicture = newPicture;
    }
    [self displayPreview];
    
}

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title
{
    if ([fPreviewWindow isVisible])
    {
        [fPreviewWindow close];
    }
    else
    {
        [self showWindow:sender];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PreviewWindowIsOpen"];
        [fPreviewWindow setAcceptsMouseMovedEvents:YES];
        isFullScreen = NO;
        scaleToScreen = NO;
        [self pictureSliderChanged:nil];
    }
    
}

- (NSString*) pictureSizeInfoString
{
    return [fInfoField stringValue];
}

- (IBAction)showPictureSettings:(id)sender
{
    [fHBController showPicturePanel:self];
}

#pragma mark Hud Control Overlay
- (void) mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];
    NSPoint mouseLoc = [theEvent locationInWindow];
    
    /* Test for mouse location to show/hide hud controls */
    if( isEncoding == NO ) {
        if( NSPointInRect( mouseLoc, [fPictureControlBox frame] ) )
        {
            [[fPictureControlBox animator] setHidden: NO];
            [self stopHudTimer];
        }
		else if( NSPointInRect( mouseLoc, [fPictureViewArea frame] ) )
        {
            [[fPictureControlBox animator] setHidden: NO];
            [self startHudTimer];
        }
        else
            [[fPictureControlBox animator] setHidden: YES];
	}
}

- (void) startHudTimer
{
	if( fHudTimer ) {
		[fHudTimer invalidate];
		[fHudTimer release];
	}
    fHudTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(hudTimerFired:) userInfo:nil repeats:YES];
    [fHudTimer retain];
}

- (void) stopHudTimer
{
    if( fHudTimer )
    {
        [fHudTimer invalidate];
        [fHudTimer release];
        fHudTimer = nil;
        hudTimerSeconds = 0;
    }
}

- (void) hudTimerFired: (NSTimer*)theTimer
{
    hudTimerSeconds++;
    if( hudTimerSeconds >= 10 ) {
        [[fPictureControlBox animator] setHidden: YES];
        [self stopHudTimer];
    }
}

#pragma mark Fullscreen Mode

- (IBAction)toggleScreenMode:(id)sender
{
    if (!isFullScreen)
    {
        [self goFullScreen:nil];
    }
    else
    {
        [self goWindowedScreen:nil];
    }
}

- (IBAction)toggleScaleToScreen:(id)sender
{
    if (scaleToScreen == YES)
    {
        scaleToScreen = NO;
        /* make sure we are set to a still preview */
        [self pictureSliderChanged:nil];
        [fScaleToScreenToggleButton setTitle:@"<->"];
    }
    else
    {
        scaleToScreen = YES;
        /* make sure we are set to a still preview */
        [self pictureSliderChanged:nil];
        [fScaleToScreenToggleButton setTitle:@">-<"];
    }
}

- (BOOL)fullScreen
{
    return isFullScreen;
}

- (IBAction)goFullScreen:(id)sender 
{ 
    // Get the screen information. 
    NSScreen* mainScreen = [fPreviewWindow screen];
    NSDictionary* screenInfo = [mainScreen deviceDescription]; 
    NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"]; 
    // Capture the screen. 
    CGDirectDisplayID displayID = (CGDirectDisplayID)[screenID longValue]; 
    CGDisplayErr err = CGDisplayCapture(displayID); 
    
    if (err == CGDisplayNoErr) 
    { 
        
        /* make sure we are set to a still preview and not scaled to screen */
        scaleToScreen = NO;
        [self pictureSliderChanged:nil];
        
        // Create the full-screen window. 
        //NSRect winRect = [mainScreen frame];
        //fPictureViewArea
        NSRect winRect = [fPictureViewArea frame];
          
        fFullScreenWindow = [[NSWindow alloc] initWithContentRect:winRect 
                                                        styleMask:NSBorderlessWindowMask 
                                                          backing:NSBackingStoreBuffered 
                                                            defer:NO 
                                                           screen:mainScreen]; 
        
        // Establish the window attributes. 
        [fFullScreenWindow setReleasedWhenClosed:NO]; 
        [fFullScreenWindow setDisplaysWhenScreenProfileChanges:YES]; 
        [fFullScreenWindow setDelegate:self]; 
        
        /* insert a view into the new window */
        [fFullScreenWindow setContentView:fPictureViewArea]; 
        [fPictureViewArea setNeedsDisplay:YES];
        
        /* Better to center the window using the screen's frame
         * and the windows origin. Note that we should take into
         * account the auto sizing and alignment that occurs in 
         * setViewSize each time the preview changes.
         * Note: by using [fFullScreenWindow screen] (instead of
         * [NSScreen mainScreen]) in referencing the screen
         * coordinates, the full screen window will show up on
         * whichever display was being used in windowed mode
         * on multi-display systems
         */
        
        NSSize screenSize = [[fFullScreenWindow screen] frame].size;
        NSSize windowSize = [fFullScreenWindow frame].size;
        NSPoint windowOrigin = [fFullScreenWindow frame].origin;
        
        /* Adjust our origin y (vertical) based on the screen height */
        windowOrigin.y += (screenSize.height - windowSize.height) / 2.0;
        windowOrigin.x += (screenSize.width - windowSize.width) / 2.0;
        
        [fFullScreenWindow setFrameOrigin:windowOrigin];
        
        /* lets kill the timer for now */
        [self stopReceivingLibhbNotifications];
        
        /* We need to retain the fPreviewWindow */
        [fPreviewWindow retain];
        
        [self setWindow:fFullScreenWindow];
        
        // The window has to be above the level of the shield window.
        int32_t shieldLevel = CGShieldingWindowLevel(); 
        
        [fFullScreenWindow setLevel:shieldLevel]; 
        
        // Show the window. 
        [fFullScreenWindow makeKeyAndOrderFront:self];
        
        
        /* Change the name of fFullScreenToggleButton appropriately */
        [fFullScreenToggleButton setTitle: @"Windowed"];
        
        /* Lets fire the timer back up for the hud controls, etc. */
        [self startReceivingLibhbNotifications];
        
        isFullScreen = YES;
        [fScaleToScreenToggleButton setHidden:NO];
        
        /* make sure we are set to a still preview */
        [self pictureSliderChanged:nil];
        
        [fFullScreenWindow setAcceptsMouseMovedEvents:YES];
        
        
        hudTimerSeconds = 0;
        [self startHudTimer];
    } 
} 

// Title-less windows normally don't receive key presses, override this
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

// Title-less windows normally can't become main which means that another
// non-fullscreen window will have the "active" titlebar in expose. Bad, fix it.
- (BOOL)canBecomeMainWindow
{
    return YES;
}


- (IBAction)goWindowedScreen:(id)sender
{
    
    /* Get the screen info to release the display but don't actually do
     * it until the windowed screen is setup.
     */
    scaleToScreen = NO;
    [self pictureSliderChanged:nil];
    [fScaleToScreenToggleButton setTitle:@"<->"];
        
    NSScreen* mainScreen = [NSScreen mainScreen]; 
    NSDictionary* screenInfo = [mainScreen deviceDescription]; 
    NSNumber* screenID = [screenInfo objectForKey:@"NSScreenNumber"];
    CGDirectDisplayID displayID = (CGDirectDisplayID)[screenID longValue]; 
    
    [fFullScreenWindow dealloc];
    [fFullScreenWindow release];
    
    
    [fPreviewWindow setContentView:fPictureViewArea]; 
    [fPictureViewArea setNeedsDisplay:YES];
    [self setWindow:fPreviewWindow];
    
    // Show the window. 
    [fPreviewWindow makeKeyAndOrderFront:self];
    
    /* Set the window back to regular level */
    [fPreviewWindow setLevel:NSNormalWindowLevel];
    
    /* Set the isFullScreen flag back to NO */
    isFullScreen = NO;
    scaleToScreen = NO;
    /* make sure we are set to a still preview */
    [self pictureSliderChanged:nil];
    [self showPreviewWindow:nil];
    
    /* Change the name of fFullScreenToggleButton appropriately */
    [fFullScreenToggleButton setTitle: @"Full Screen"];
    // [fScaleToScreenToggleButton setHidden:YES];
    /* set the picture settings pallete back to normal level */
    [fHBController picturePanelWindowed];
    
    /* Release the display now that the we are back in windowed mode */
    CGDisplayRelease(displayID);
    
    [fPreviewWindow setAcceptsMouseMovedEvents:YES];
    //[fFullScreenWindow setAcceptsMouseMovedEvents:NO];
    
    hudTimerSeconds = 0;
    [self startHudTimer];
    
}


#pragma mark Still Preview Image Processing


// This function converts an image created by libhb (specified via pictureIndex) into
// an NSImage suitable for the GUI code to use. If removeBorders is YES,
// makeImageForPicture crops the image generated by libhb stripping off the gray
// border around the content. This is the low-level method that generates the image.
// -imageForPicture calls this function whenever it can't find an image in its cache.
+ (NSImage *) makeImageForPicture: (int)pictureIndex
                libhb:(hb_handle_t*)handle
                title:(hb_title_t*)title
{
    static uint8_t * buffer;
    static int bufferSize;

    // Make sure we have a big enough buffer to receive the image from libhb. libhb
    int dstWidth = title->job->width;
    int dstHeight = title->job->height;
        
    int newSize;
    newSize = dstWidth * dstHeight * 4;
    if( bufferSize < newSize )
    {
        bufferSize = newSize;
        buffer     = (uint8_t *) realloc( buffer, bufferSize );
    }

    hb_get_preview( handle, title, pictureIndex, buffer );

    // Create an NSBitmapImageRep and copy the libhb image into it, converting it from
    // libhb's format to one suitable for NSImage. Along the way, we'll strip off the
    // border around libhb's image.
        
    // The image data returned by hb_get_preview is 4 bytes per pixel, BGRA format.
    // Alpha is ignored.
        
    NSBitmapFormat bitmapFormat = (NSBitmapFormat)NSAlphaFirstBitmapFormat;
    NSBitmapImageRep * imgrep = [[[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:nil
            pixelsWide:dstWidth
            pixelsHigh:dstHeight
            bitsPerSample:8
            samplesPerPixel:3   // ignore alpha
            hasAlpha:NO
            isPlanar:NO
            colorSpaceName:NSCalibratedRGBColorSpace
            bitmapFormat:bitmapFormat
            bytesPerRow:dstWidth * 4
            bitsPerPixel:32] autorelease];

    UInt32 * src = (UInt32 *)buffer;
    UInt32 * dst = (UInt32 *)[imgrep bitmapData];
    for (int r = 0; r < dstHeight; r++)
    {
        for (int c = 0; c < dstWidth; c++)
#if TARGET_RT_LITTLE_ENDIAN
            *dst++ = Endian32_Swap(*src++);
#else
            *dst++ = *src++;
#endif
    }

    NSImage * img = [[[NSImage alloc] initWithSize: NSMakeSize(dstWidth, dstHeight)] autorelease];
    [img addRepresentation:imgrep];

    return img;
}

// Returns the preview image for the specified index, retrieving it from its internal
// cache or by calling makeImageForPicture if it is not cached. Generally, you should
// use imageForPicture so that images are cached. Calling makeImageForPicture will
// always generate a new copy of the image.
- (NSImage *) imageForPicture: (int) pictureIndex
{
    // The preview for the specified index may not currently exist, so this method
    // generates it if necessary.
    NSString * key = [NSString stringWithFormat:@"%d", pictureIndex];
    NSImage * theImage = [fPicturePreviews objectForKey:key];
    if (!theImage)
    {
        theImage = [PreviewController makeImageForPicture:pictureIndex libhb:fHandle title:fTitle];
        [fPicturePreviews setObject:theImage forKey:key];
    }
    return theImage;
}

// Purges all images from the cache. The next call to imageForPicture will cause a new
// image to be generated.
- (void) purgeImageCache
{
    [fPicturePreviews removeAllObjects];
}

 

#pragma mark Movie Preview
- (IBAction) createMoviePreview: (id) sender
{
    
    
    /* Lets make sure the still picture previews are showing in case
     * there is currently a movie showing */
    [self pictureSliderChanged:nil];
    
    /* Rip or Cancel ? */
    hb_state_t s;
    hb_get_state2( fPreviewLibhb, &s );
    
    if(sender == fCancelPreviewMovieButton && (s.state == HB_STATE_WORKING || s.state == HB_STATE_PAUSED))
	{
        hb_stop( fPreviewLibhb );
        [fPictureView setHidden:NO];
        [fMovieView pause:nil];
        [fMovieView setHidden:YES];
		[fMovieView setMovie:nil];
        [fPictureSlider setHidden:NO];
        isEncoding = NO;
        
        return;
    }
    
    
    /* we use controller.mm's prepareJobForPreview to go ahead and set all of our settings
     * however, we want to use a temporary destination field of course
     * so that we do not put our temp preview in the users chosen
     * directory */
    
    hb_job_t * job = fTitle->job;
    
    /* We run our current setting through prepeareJob in Controller.mm
     * just as if it were a regular encode */
    
    [fHBController prepareJobForPreview];
    
    /* Destination file. We set this to our preview directory
     * changing the extension appropriately.*/
    if (fTitle->job->mux == HB_MUX_MP4) // MP4 file
    {
        /* we use .m4v for our mp4 files so that ac3 and chapters in mp4 will play properly */
        fPreviewMoviePath = @"~/Library/Application Support/HandBrake/Previews/preview_temp.m4v";
    }
    else if (fTitle->job->mux == HB_MUX_MKV) // MKV file
    {
        fPreviewMoviePath = @"~/Library/Application Support/HandBrake/Previews/preview_temp.mkv";
    }
    else if (fTitle->job->mux == HB_MUX_AVI) // AVI file
    {
        fPreviewMoviePath = @"~/Library/Application Support/HandBrake/Previews/preview_temp.avi";
    }
    else if (fTitle->job->mux == HB_MUX_OGM) // OGM file
    {
        fPreviewMoviePath = @"~/Library/Application Support/HandBrake/Previews/preview_temp.ogm";
    }
    
    fPreviewMoviePath = [[fPreviewMoviePath stringByExpandingTildeInPath]retain];
    
    /* See if there is an existing preview file, if so, delete it */
    if( ![[NSFileManager defaultManager] fileExistsAtPath:fPreviewMoviePath] )
    {
        [[NSFileManager defaultManager] removeFileAtPath:fPreviewMoviePath
                                                 handler:nil];
    }
    
    /* We now direct our preview encode to fPreviewMoviePath */
    fTitle->job->file = [fPreviewMoviePath UTF8String];
    
    /* We use our advance pref to determine how many previews to scan */
    int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
    job->start_at_preview = fPicture + 1;
    job->seek_points = hb_num_previews;
    
    /* we use the preview duration popup to get the specified
     * number of seconds for the preview encode.
     */
    
    job->pts_to_stop = [[fPreviewMovieLengthPopUp titleOfSelectedItem] intValue] * 90000LL;
    
    /* lets go ahead and send it off to libhb
     * Note: unlike a full encode, we only send 1 pass regardless if the final encode calls for 2 passes.
     * this should suffice for a fairly accurate short preview and cuts our preview generation time in half.
     * However we also need to take into account the indepth scan for subtitles.
     */
    /*
     * If scanning we need to do some extra setup of the job.
     */
    if( job->indepth_scan == 1 )
    {
        char *x264opts_tmp;
        
        /*
         * When subtitle scan is enabled do a fast pre-scan job
         * which will determine which subtitles to enable, if any.
         */
        job->pass = -1;
        x264opts_tmp = job->x264opts;
        
        job->x264opts = NULL;
        job->indepth_scan = 1;  
        /*
         * Add the pre-scan job
         */
        hb_add( fPreviewLibhb, job );
        job->x264opts = x264opts_tmp;
    }                  
    /* Go ahead and perform the actual encoding preview scan */
    job->indepth_scan = 0;
    job->pass = 0;
    hb_add( fPreviewLibhb, job );
    
    [fEncodingControlBox setHidden: NO];
    [fPictureControlBox setHidden: YES];
    
    [fMovieCreationProgressIndicator setHidden: NO];
    [fPreviewMovieStatusField setHidden: NO];
    
    isEncoding = YES;

    /* Let fPreviewLibhb do the job */
    hb_start( fPreviewLibhb );
	
}

- (void) startReceivingLibhbNotifications
{
    if (!fLibhbTimer)
    {
        fLibhbTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(libhbTimerFired:) userInfo:nil repeats:YES];
        [fLibhbTimer retain];
    }
}

- (void) stopReceivingLibhbNotifications
{
    if (fLibhbTimer)
    {
        [fLibhbTimer invalidate];
        [fLibhbTimer release];
        fLibhbTimer = nil;
    }
}
- (void) libhbTimerFired: (NSTimer*)theTimer
{
    hb_state_t s;
    hb_get_state( fPreviewLibhb, &s );
    [self libhbStateChanged: s];
    
}

- (void) libhbStateChanged: (hb_state_t &)state
{
    switch( state.state )
    {
        case HB_STATE_IDLE:
        case HB_STATE_SCANNING:
        case HB_STATE_SCANDONE:
            break;
            
        case HB_STATE_WORKING:
        {
#define p state.param.working
            
            NSMutableString * string;
			/* Update text field */
			string = [NSMutableString stringWithFormat: NSLocalizedString( @"Encoding preview:  %.2f %%", @"" ), 100.0 * p.progress];
            
			if( p.seconds > -1 )
            {
                [string appendFormat:
                 NSLocalizedString( @" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)", @"" ),
                 p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
            }
            [fPreviewMovieStatusField setStringValue: string];
            
            [fMovieCreationProgressIndicator setIndeterminate: NO];
            /* Update slider */
			[fMovieCreationProgressIndicator setDoubleValue: 100.0 * p.progress];
            
            [fCreatePreviewMovieButton setTitle: @"Cancel Preview"];
            
            break;
            
        }
#undef p
            
#define p state.param.muxing            
        case HB_STATE_MUXING:
        {
            // Update fMovieCreationProgressIndicator
            [fMovieCreationProgressIndicator setIndeterminate: YES];
            [fMovieCreationProgressIndicator startAnimation: nil];
            [fPreviewMovieStatusField setStringValue: [NSString stringWithFormat:
                                         NSLocalizedString( @"Muxing Preview ...", @"" )]];
            break;
        }
#undef p			
        case HB_STATE_PAUSED:
            [fMovieCreationProgressIndicator stopAnimation: nil];
            break;
			
        case HB_STATE_WORKDONE:
        {
            // Delete all remaining jobs since libhb doesn't do this on its own.
            hb_job_t * job;
            while( ( job = hb_job(fPreviewLibhb, 0) ) )
                hb_rem( fHandle, job );
            
            [fPreviewMovieStatusField setStringValue: @""];
            [fPreviewMovieStatusField setHidden: YES];
            
            [fMovieCreationProgressIndicator stopAnimation: nil];
            [fMovieCreationProgressIndicator setHidden: YES];
            [fEncodingControlBox setHidden: YES];
            isEncoding = NO;
            /* we make sure the picture slider and preview match */
            [self pictureSliderChanged:nil];

            // Show the movie view
            [self showMoviePreview:fPreviewMoviePath];
            [fCreatePreviewMovieButton setTitle: @"Live Preview"];

            break;
        }
    }
}

- (IBAction) showMoviePreview: (NSString *) path
{
    /* Since the gray background for the still images is part of
     * fPictureView, lets leave the picture view visible and postion
     * the fMovieView over the image portion of fPictureView so
     * we retain the gray cropping border  we have already established
     * with the still previews
     */

    /* Load the new movie into fMovieView */
    if (path) {
		QTMovie * aMovie;
		NSError	 *outError;
		NSURL *movieUrl = [NSURL fileURLWithPath:path];
		NSDictionary *movieAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
										 movieUrl, QTMovieURLAttribute,
										 [NSNumber numberWithBool:NO], QTMovieAskUnresolvedDataRefsAttribute,
										 [NSNumber numberWithBool:YES], @"QTMovieOpenForPlaybackAttribute",
										 [NSNumber numberWithBool:NO], @"QTMovieOpenAsyncRequiredAttribute",								
										 [NSNumber numberWithBool:NO], @"QTMovieOpenAsyncOKAttribute",
										 QTMovieApertureModeClean, QTMovieApertureModeAttribute,
										 nil];

        aMovie = [[[QTMovie alloc] initWithAttributes:movieAttributes error:&outError] autorelease];

		if (!aMovie) {
			NSLog(@"Unable to open movie");
		}
        else {
            NSRect movieBounds;
            /* we get some size information from the preview movie */
            NSSize movieSize= [[aMovie attributeForKey:QTMovieNaturalSizeAttribute] sizeValue];
            movieBounds = [fMovieView movieBounds];
            movieBounds.size.height = movieSize.height;
            
            if ([fMovieView isControllerVisible]) {
                CGFloat controllerBarHeight = [fMovieView controllerBarHeight];
                if ( controllerBarHeight != 0 ) //Check if QTKit return a real value or not.
                    movieBounds.size.height += controllerBarHeight;
                else
                    movieBounds.size.height += 15;
            }
            
            movieBounds.size.width = movieSize.width;
            
            /* We need to find out if the preview movie needs to be scaled down so
             * that it doesn't overflow our available viewing container (just like for image
             * in -displayPreview) for HD sources, etc. [fPictureViewArea frame].size.height*/
            if( (movieBounds.size.height) > [fPictureViewArea frame].size.height || scaleToScreen == YES )
            {
                /* The preview movie would be larger than the available viewing area
                 * in the preview movie, so we go ahead and scale it down to the same size
                 * as the still preview  or we readjust our window to allow for the added height if need be
                 */
                NSSize displaySize = NSMakeSize( ( CGFloat ) movieBounds.size.width, ( CGFloat ) movieBounds.size.height );
                NSSize viewSize = [self optimalViewSizeForImageSize:displaySize];
                if( [self viewNeedsToResizeToSize:viewSize] ) {
                    [self resizeSheetForViewSize:viewSize];
                    [self setViewSize:viewSize];
                }
                [fMovieView setFrameSize:viewSize];
            }
            else
            {
                /* Since the preview movie is smaller than the available viewing area
                 * we can go ahead and use the preview movies native size */
                [fMovieView setFrameSize:movieBounds.size];
            }
            
            //lets reposition the movie if need be
            
            NSPoint origin = [fPictureViewArea frame].origin;
            origin.x += trunc( ( [fPictureViewArea frame].size.width -
                                [fMovieView frame].size.width ) / 2.0 );
            /* We need to detect whether or not we are currently less than the available height.*/
            if( movieBounds.size.height < [fPictureView frame].size.height )
            {
                /* If we are, we are adding 15 to the height to allow for the controller bar so
                 * we need to subtract half of that for the origin.y to get the controller bar
                 * below the movie to it lines up vertically with where our still preview was
                 */
                origin.y += trunc( ( ( [fPictureViewArea frame].size.height -
                                      [fMovieView frame].size.height ) / 2.0 ) - 7.5 );
            }
            else
            {
                /* if we are >= to the height of the picture view area, the controller bar
                 * gets taken care of with picture resizing, so we do not want to offset the height
                 */
                origin.y += trunc( ( [fPictureViewArea frame].size.height -
                                    [fMovieView frame].size.height ) / 2.0 );
            }
            [fMovieView setFrameOrigin:origin];
            [fMovieView setMovie:aMovie];
            [fMovieView setHidden:NO];
            // to actually play the movie
            [fMovieView play:aMovie];
        }
    }
    isEncoding = NO;
}

@end

@implementation PreviewController (Private)

//
// -[PictureController(Private) optimalViewSizeForImageSize:]
//
// Given the size of the preview image to be shown, returns the best possible
// size for the view.
//
- (NSSize)optimalViewSizeForImageSize: (NSSize)imageSize
{
    // The min size is 320x240
    CGFloat minWidth = 480.0;
    CGFloat minHeight = 360.0;

    NSSize screenSize = [[NSScreen mainScreen] frame].size;
    NSSize sheetSize = [[self window] frame].size;
    NSSize viewAreaSize = [fPictureViewArea frame].size;
    CGFloat paddingX = sheetSize.width - viewAreaSize.width;
    CGFloat paddingY = sheetSize.height - viewAreaSize.height;
    CGFloat maxWidth;
    CGFloat maxHeight;
    
    if (isFullScreen)
    {
        /* We are in full screen mode so lets use the full screen if we need to */
        maxWidth =  screenSize.width - paddingX;
        maxHeight = screenSize.height - paddingY;
    }
    else
    {
        // The max size of the view is when the sheet is taking up 85% of the screen.
        maxWidth = (0.85 * screenSize.width) - paddingX;
        maxHeight = (0.85 * screenSize.height) - paddingY;
    }
    
    NSSize resultSize = imageSize;
    
    // Its better to have a view that's too small than a view that's too big, so
    // apply the maximum constraints last.
    if( resultSize.width < minWidth )
    {
        resultSize.height *= (minWidth / resultSize.width);
        resultSize.width = minWidth;
    }
    if( resultSize.height < minHeight )
    {
        resultSize.width *= (minHeight / resultSize.height);
        resultSize.height = minHeight;
    }
    if( resultSize.width > maxWidth )
    {
        resultSize.height *= (maxWidth / resultSize.width);
        resultSize.width = maxWidth;
    }
    if( resultSize.height > maxHeight )
    {
        resultSize.width *= (maxHeight / resultSize.height);
        resultSize.height = maxHeight;
    }
    
    if (scaleToScreen == YES)
    {
        CGFloat screenAspect;
        CGFloat viewAreaAspect; 
        //note, a mbp 15" at 1440 x 900 is a 1.6 ar
        screenAspect = screenSize.width / screenSize.height;
        
        // Note, a standard dvd will use 720 x 480 which is a 1.5
        viewAreaAspect = viewAreaSize.width / viewAreaSize.height;
        
        if (screenAspect < viewAreaAspect)
        {
            resultSize.width = screenSize.width;
            resultSize.height = (screenSize.width / viewAreaAspect);
        }
        else
        {
            resultSize.height = screenSize.height;
            resultSize.width = resultSize.height * viewAreaAspect;
        }
        
    }

      return resultSize;

    
}

//
// -[PictureController(Private) resizePanelForViewSize:animate:]
//
// Resizes the entire window to accomodate a view of a particular size.
//
- (void)resizeSheetForViewSize: (NSSize)viewSize
{
    // Figure out the deltas for the new frame area
    NSSize currentSize = [fPictureViewArea frame].size;
    CGFloat deltaX = viewSize.width - currentSize.width;
    CGFloat deltaY = viewSize.height - currentSize.height;
    
    // Now resize the whole panel by those same deltas, but don't exceed the min
    NSRect frame = [[self window] frame];
    NSSize maxSize = [[self window] maxSize];
    NSSize minSize = [[self window] minSize];
    frame.size.width += deltaX;
    frame.size.height += deltaY;
    if( frame.size.width < minSize.width )
    {
        frame.size.width = minSize.width;
    }
    
    if( frame.size.height < minSize.height )
    {
        frame.size.height = minSize.height;
    }
    
    
    // But now the sheet is off-center, so also shift the origin to center it and
    // keep the top aligned.
    if( frame.size.width != [[self window] frame].size.width )
        frame.origin.x -= (deltaX / 2.0);
    
    if (isFullScreen)
    {
        if( frame.size.height != [[self window] frame].size.height )
        {
            frame.origin.y -= (deltaY / 2.0);
        }
        else
        {
            if( frame.size.height != [[self window] frame].size.height )
                frame.origin.y -= deltaY;
        }
        
        [[self window] setFrame:frame display:YES animate:NO];
    }
    else
    {
        /* Since upon launch we can open up the preview window if it was open
         * the last time we quit (and at the size it was) we want to make
         * sure that upon resize we do not have the window off the screen
         * So check the origin against the screen origin and adjust if
         * necessary.
         */
        NSSize screenSize = [[[self window] screen] frame].size;
        NSPoint screenOrigin = [[[self window] screen] frame].origin;
        /* our origin is off the screen to the left*/
        if (frame.origin.x < screenOrigin.x)
        {
            /* so shift our origin to the right */
            frame.origin.x = screenOrigin.x;
        }
        else if ((frame.origin.x + frame.size.width) > (screenOrigin.x + screenSize.width))
        {
            /* the right side of the preview is off the screen, so shift to the left */
            frame.origin.x = (screenOrigin.x + screenSize.width) - frame.size.width;
        }
        
        [[self window] setFrame:frame display:YES animate:YES];
    }
    
}

//
// -[PictureController(Private) setViewSize:]
//
// Changes the view's size and centers it vertically inside of its area.
// Assumes resizeSheetForViewSize: has already been called.
//
- (void)setViewSize: (NSSize)viewSize
{   
    /* special case for scaleToScreen */
    if (scaleToScreen == YES)
    {
        /* for scaleToScreen, we expand the fPictureView to fit the entire screen */
        NSSize areaSize = [fPictureViewArea frame].size;
        CGFloat viewSizeAspect = viewSize.width / viewSize.height;
        if (viewSizeAspect > 1.0) // we are wider than taller, so expand the width to fill the area and scale the height
        {
            viewSize.width = areaSize.width;
            viewSize.height = viewSize.width / viewSizeAspect;
        }
        else
        {
            viewSize.height = areaSize.height;
            viewSize.width = viewSize.height * viewSizeAspect;
        }
        
    }

    [fPictureView setFrameSize:viewSize];

    // center it vertically and horizontally
    NSPoint origin = [fPictureViewArea frame].origin;
    origin.y += ([fPictureViewArea frame].size.height -
                 [fPictureView frame].size.height) / 2.0;
    
    origin.x += ([fPictureViewArea frame].size.width -
                 [fPictureView frame].size.width) / 2.0; 

    origin.x = floor( origin.x );
    origin.y = floor( origin.y );
    
    [fPictureView setFrameOrigin:origin];
}


- (BOOL)viewNeedsToResizeToSize: (NSSize)newSize
{
    NSSize viewSize = [fPictureViewArea frame].size;
    return (newSize.width != viewSize.width || newSize.height != viewSize.height);
}

@end
