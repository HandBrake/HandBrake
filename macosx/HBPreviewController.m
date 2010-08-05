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
    [fMovieTimer invalidate];
    [fMovieTimer release];
    [fMovieView setHidden:YES];
	[fMovieView setMovie:nil];

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
        [[NSFileManager defaultManager] removeItemAtPath:fPreviewMoviePath error:nil];
        [fPreviewMoviePath release];
    }    
    
    [fLibhbTimer invalidate];
    [fLibhbTimer release];
    
    [fHudTimer invalidate];
    [fHudTimer release];
    
    [fMovieTimer invalidate];
    [fMovieTimer release];
    
    [fPicturePreviews release];
    [fFullScreenWindow release];
    
    hb_close(&fPreviewLibhb);
    
    [self removeMovieCallbacks];
    
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
     aMovie = nil;
    [fMovieView pause:nil];
    [fMovieView setHidden:YES];
	[fMovieView setMovie:nil];
    [fMovieCreationProgressIndicator stopAnimation: nil];
    [fMovieCreationProgressIndicator setHidden: YES];
    [fMoviePlaybackControlBox setHidden: YES];
    if( fMovieTimer )
    {
        [self stopMovieTimer];
    }
    [fPictureControlBox setHidden: NO];
    
    [fPictureView setHidden:NO];
    
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
        
        displaySize.width = fTitle->job->anamorphic.dar_width + fTitle->job->crop[2] + fTitle->job->crop[3] ;
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
    [self resizeSheetForViewSize:viewSize];

    NSSize windowSize = [[self window] frame].size;    
    
    if (scaleToScreen == YES)
    {
        /* Note: this should probably become a utility function */
        /* We are in Scale To Screen mode so, we have to get the ratio for height and width against the window
         *size so we can scale from there.
         */
        CGFloat deltaWidth = imageScaledSize.width / displaySize.width;
        CGFloat deltaHeight = imageScaledSize.height /displaySize.height;
        NSSize windowSize = [[self window] frame].size;  
        CGFloat pictureAspectRatio = imageScaledSize.width / imageScaledSize.height;
        
        /* Set our min size to the storage size */
        NSSize minSize;
        minSize.width = fTitle->width;
        minSize.height = fTitle->height;

        /* Set delta's based on minimum size */
        if (imageScaledSize.width <  minSize.width)
        {
            deltaWidth = imageScaledSize.width / minSize.width;
        }
        else
        {
            deltaWidth = 1.0;
        }
        
        if (imageScaledSize.height <  minSize.height)
        {
            deltaHeight =  imageScaledSize.height / minSize.height;
        }
        else
        {
            deltaHeight = 1.0;
        }
        
        /* Now apply our deltas to the full screen view */
        if (pictureAspectRatio > 1.0) // we are wider than taller, so expand the width to fill the area and scale the height
        {
            viewSize.width = windowSize.width * deltaWidth;
            viewSize.height = viewSize.width / pictureAspectRatio;
            
        }
        else
        {
            viewSize.height = windowSize.height * deltaHeight; 
            viewSize.width = viewSize.height * pictureAspectRatio;
        }
        
    }
    else
    {
        viewSize.width = viewSize.width - (viewSize.width - imageScaledSize.width);
        viewSize.height = viewSize.height - (viewSize.height - imageScaledSize.height);
        
        if (fTitle->width > windowSize.width || fTitle->height > windowSize.height)
        {
            CGFloat viewSizeAspect = viewSize.width / viewSize.height;
            if (viewSizeAspect > 1.0) // we are wider than taller, so expand the width to fill the area and scale the height
            {
                viewSize.width = viewSize.width * (windowSize.width / fTitle->width) ;
                viewSize.height = viewSize.width / viewSizeAspect;
            }
            else
            {
                viewSize.height = viewSize.height * (windowSize.height / fTitle->height);
                viewSize.width = viewSize.height * viewSizeAspect;
            }
        }
        
    }
    
    [self setViewSize:viewSize];
    
    /* relocate our hud origins as per setViewSize */
    NSPoint hudControlBoxOrigin = [fPictureControlBox frame].origin;
    hudControlBoxOrigin.y = ([[self window] frame].size.height / 2) - (viewSize.height / 2);
    hudControlBoxOrigin.x = ([[self window] frame].size.width / 2) - ([fPictureControlBox frame].size.width / 2);
    [fPictureControlBox setFrameOrigin:hudControlBoxOrigin];
    [fEncodingControlBox setFrameOrigin:hudControlBoxOrigin];
    [fMoviePlaybackControlBox setFrameOrigin:hudControlBoxOrigin];


    NSString *scaleString;
    CGFloat scale = ( ( CGFloat )[fPictureView frame].size.width) / ( ( CGFloat )imageScaledSize.width);
    if (scale * 100.0 != 100)
    {
        scaleString = [NSString stringWithFormat:
                       NSLocalizedString( @" (%.0f%% actual size)",
                                         @"String shown when a preview is scaled" ), scale * 100.0];
    }
    else
    {
        scaleString = @"(Actual size)";
    }
    
    if (scaleToScreen == YES)
    {
        scaleString = [scaleString stringByAppendingString:@" Scaled To Screen"];
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
    if( isEncoding == NO ) 
    {
        /* Since we are not encoding, verify which control hud to show
         * or hide based on aMovie ( aMovie indicates we need movie controls )
         */
        NSBox           * hudBoxToShow;
        if ( aMovie == nil ) // No movie loaded up
        {
            hudBoxToShow = fPictureControlBox;
        }
        else // We have a movie
        {
            hudBoxToShow = fMoviePlaybackControlBox;
        }
        
        if( NSPointInRect( mouseLoc, [fPictureControlBox frame] ) )
        {
            [[hudBoxToShow animator] setHidden: NO];
            [self stopHudTimer];
        }
		else if( NSPointInRect( mouseLoc, [fPictureViewArea frame] ) )
        {
            [[hudBoxToShow animator] setHidden: NO];
            [self startHudTimer];
        }
        else
        {
            [[hudBoxToShow animator] setHidden: YES];
        }
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
    if( hudTimerSeconds >= 10 ) 
    {
        /* Regardless which control box is active, after the timer
         * period we want either one to fade to hidden.
         */
        [[fPictureControlBox animator] setHidden: YES];
        [[fMoviePlaybackControlBox animator] setHidden: YES];
        [self stopHudTimer];
    }
}



- (IBAction)toggleScaleToScreen:(id)sender
{
    if (scaleToScreen == YES)
    {
        scaleToScreen = NO;
        /* make sure we are set to a still preview */
        [self pictureSliderChanged:nil];
        [fScaleToScreenToggleButton setTitle:@"Scale To Screen"];
    }
    else
    {
        scaleToScreen = YES;
        /* make sure we are set to a still preview */
        [self pictureSliderChanged:nil];
        [fScaleToScreenToggleButton setTitle:@"Actual Scale"];
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
    //isFullScreen = NO;
    scaleToScreen = NO;
    /* make sure we are set to a still preview */
    [self pictureSliderChanged:nil];
    [self showPreviewWindow:nil];
    
    /* Change the name of fFullScreenToggleButton appropriately */
    //[fFullScreenToggleButton setTitle: @"Full Screen"];
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
    int r, c;
    for (r = 0; r < dstHeight; r++)
    {
        for (c = 0; c < dstWidth; c++)
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

- (IBAction) cancelCreateMoviePreview: (id) sender
{
    
    hb_state_t s;
    hb_get_state2( fPreviewLibhb, &s );
    
    if(isEncoding && (s.state == HB_STATE_WORKING || s.state == HB_STATE_PAUSED))
    {
        hb_stop( fPreviewLibhb );
        [fPictureView setHidden:NO];
        [fMovieView pause:nil];
        [fMovieView setHidden:YES];
		[fMovieView setMovie:nil];
        [fPictureSlider setHidden:NO];
        isEncoding = NO;
        
        [self pictureSliderChanged:nil];
        
        return;
    }
    
}

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
        [[NSFileManager defaultManager] removeItemAtPath:fPreviewMoviePath error:nil];
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

- (void) libhbStateChanged: (hb_state_t)state
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
            [fPictureControlBox setHidden: YES];
            isEncoding = NO;

            // Show the movie view
            [self showMoviePreview:fPreviewMoviePath];
            [fCreatePreviewMovieButton setTitle: @"Live Preview"];

            break;
        }
    }
}

- (IBAction) toggleMoviePreviewPlayPause: (id) sender
{
    /* make sure a movie is even loaded up */
    if (aMovie != nil)
    {
        /* For some stupid reason there is no "isPlaying" method for a QTMovie
         * object, given that, we detect the rate to determine whether the movie
         * is playing or not.
         */
        if ([aMovie rate] != 0) // we are playing 
        {
            [fMovieView pause:aMovie];
            [fPlayPauseButton setTitle: @">"];
        }
        else // we are paused or stopped
        {
            [fMovieView play:aMovie];
            [fPlayPauseButton setTitle: @"||"];   
        }
    }
    
}

- (IBAction) moviePlaybackGoToBeginning: (id) sender
{
    /* make sure a movie is even loaded up */
    if (aMovie != nil)
    {
        [fMovieView gotoBeginning:aMovie];
     }
    
}

- (IBAction) moviePlaybackGoToEnd: (id) sender
{
    /* make sure a movie is even loaded up */
    if (aMovie != nil)
    {
        [fMovieView gotoEnd:aMovie];
     }
    
}

- (IBAction) moviePlaybackGoBackwardOneFrame: (id) sender
{
    /* make sure a movie is even loaded up */
    if (aMovie != nil)
    {
        [fMovieView pause:aMovie]; // Pause the movie
        [fMovieView stepBackward:aMovie];
     }
    
}

- (IBAction) moviePlaybackGoForwardOneFrame: (id) sender
{
    /* make sure a movie is even loaded up */
    if (aMovie != nil)
    {
        [fMovieView pause:aMovie]; // Pause the movie
        [fMovieView stepForward:aMovie];
     }
    
}


- (void) startMovieTimer
{
	if( fMovieTimer ) {
		[fMovieTimer invalidate];
		[fMovieTimer release];
	}
    fMovieTimer = [NSTimer scheduledTimerWithTimeInterval:0.10 target:self selector:@selector(movieTimerFired:) userInfo:nil repeats:YES];
    [fMovieTimer retain];
}

- (void) stopMovieTimer
{
    if( fMovieTimer )
    {
        [fMovieTimer invalidate];
        [fMovieTimer release];
        fMovieTimer = nil;
    }
}

- (void) movieTimerFired: (NSTimer*)theTimer
{
     if (aMovie != nil)
    {
        [self adjustPreviewScrubberForCurrentMovieTime];
        [fMovieInfoField setStringValue: [NSString stringWithFormat:NSLocalizedString( @"%@", @"" ),[self calculatePlaybackSMTPETimecodeForDisplay]]];    
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
    if (path) 
    {
		//QTMovie * aMovie;
		NSError	 *outError;
		NSURL *movieUrl = [NSURL fileURLWithPath:path];
		NSDictionary *movieAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
										 movieUrl, QTMovieURLAttribute,
										 [NSNumber numberWithBool:NO], QTMovieAskUnresolvedDataRefsAttribute,
										 [NSNumber numberWithBool:YES], @"QTMovieOpenForPlaybackAttribute",
										 [NSNumber numberWithBool:NO], @"QTMovieOpenAsyncRequiredAttribute",								
										 [NSNumber numberWithBool:NO], @"QTMovieOpenAsyncOKAttribute",
                                         [NSNumber numberWithBool:YES], @"QTMovieIsSteppableAttribute",
										 QTMovieApertureModeClean, QTMovieApertureModeAttribute,
										 nil];
        
        aMovie = [[[QTMovie alloc] initWithAttributes:movieAttributes error:&outError] autorelease];
        
        
		if (!aMovie) 
        {
			NSLog(@"Unable to open movie");
		}
        else 
        {
            NSRect movieBounds;
            /* we get some size information from the preview movie */
            NSSize movieSize= [[aMovie attributeForKey:QTMovieNaturalSizeAttribute] sizeValue];
            movieBounds = [fMovieView movieBounds];
            movieBounds.size.height = movieSize.height;
            /* We also get our view size to use for scaling fMovieView's size */
            NSSize scaledMovieViewSize = [fPictureView frame].size;
            [fMovieView setControllerVisible:FALSE];
            if ([fMovieView isControllerVisible]) 
            {
                CGFloat controllerBarHeight = [fMovieView controllerBarHeight];
                if ( controllerBarHeight != 0 ) //Check if QTKit return a real value or not.
                {
                    movieBounds.size.height += controllerBarHeight;
                    scaledMovieViewSize.height += controllerBarHeight;
                }
                else
                {
                    movieBounds.size.height += 15;
                    scaledMovieViewSize.height += 15;
                }
            }
            
            movieBounds.size.width = movieSize.width;
            
            /* we need to account for an issue where the scaledMovieViewSize > the window size */
            if (scaledMovieViewSize.height > [[self window] frame].size.height)
            {
                [fHBController writeToActivityLog: "showMoviePreview: Our window is not tall enough to show the controller bar ..."];
            }
            
            
            
            /* Scale the fMovieView to scaledMovieViewSize */
            [fMovieView setFrameSize:scaledMovieViewSize];
            
            /*set our origin try using fPictureViewArea or fPictureView */
            NSPoint origin = [fPictureView frame].origin;
            origin.x += trunc( ( [fPictureView frame].size.width -
                                [fMovieView frame].size.width ) / 2.0 );
            origin.y += trunc( ( ( [fPictureView frame].size.height -
                                      [fMovieView frame].size.height ) / 2.0 ) - 7.5 );

            [fMovieView setFrameOrigin:origin];
            [fMovieView setMovie:aMovie];
            [fMovieView setHidden:NO];
            [fMoviePlaybackControlBox setHidden: NO];
            [fPictureControlBox setHidden: YES];
            
            // to actually play the movie
            
            [self initPreviewScrubberForMovie];
            [self startMovieTimer];
            /* Install amovie notifications */
            [aMovie setDelegate:self];
            [self installMovieCallbacks];
            [fMovieView play:aMovie];

        }
    }
    isEncoding = NO;
}
#pragma mark *** Movie Playback Scrubber and time code methods ***

/* Since MacOSX Leopard QTKit has taken over some responsibility for assessing movie playback
 * information from the old QuickTime carbon api ( time code information as well as fps, etc.).
 * However, the QTKit devs at apple were not really big on documentation and further ...
 * QuickTimes ability to playback HB's largely variable framerate output makes perfectly frame
 * accurate information at best convoluted. Still, for the purpose of a custom hud based custom
 * playback scrubber slider this has so far proven to be as accurate as I have found. To say it
 * could use some better accuracy is not understating it enough probably.
 * Most of this was gleaned from this obscure Apple Mail list thread:
 * http://www.mailinglistarchive.com/quicktime-api@lists.apple.com/msg05642.html
 * Now as we currently do not show a QTKit control bar with scrubber for display sizes > container
 * size, this seems to facilitate playback control from the HB custom HUD controller fairly close
 * to the built in controller bar.
 * Further work needs to be done to try to get accurate frame by frame playback display if we want it.
 * Note that the keyboard commands for frame by frame step through etc. work as always.
 */ 

// Returns a human readable string from the currentTime of movie playback
- (NSString*) calculatePlaybackSMTPETimecodeForDisplay
{
    QTTime time = [aMovie currentTime];
    
    NSString *smtpeTimeCodeString;
    int days, hour, minute, second, frame;
    long long result;
    
    result = time.timeValue / time.timeScale; // second
    frame = (time.timeValue % time.timeScale) / 100;
    
    second = result % 60;
    
    result = result / 60; // minute
    minute = result % 60;
    
    result = result / 60; // hour
    hour = result % 24;	 
    days = result;
    
    smtpeTimeCodeString = [NSString stringWithFormat:@"Time: %02d:%02d:%02d", hour, minute, second]; // hh:mm:ss
    return smtpeTimeCodeString;
    
}


// Initialize the preview scrubber min/max to appropriate values for the current movie
-(void) initPreviewScrubberForMovie
{
    if (aMovie)
    {
        
        QTTime duration = [aMovie duration];
        float result = duration.timeValue / duration.timeScale;
        
        [fMovieScrubberSlider setMinValue:0.0];
        [fMovieScrubberSlider setMaxValue: (float)result];
        [fMovieScrubberSlider setFloatValue: 0.0];
    }
}


-(void) adjustPreviewScrubberForCurrentMovieTime
{
    if (aMovie)
    {
        QTTime time = [aMovie currentTime];
        
        float result = (float)time.timeValue / (float)time.timeScale;;
        [fMovieScrubberSlider setFloatValue:result];
    }
}

- (IBAction) previewScrubberChanged: (id) sender
{
    if (aMovie)
    {
        [fMovieView pause:aMovie]; // Pause the movie
        QTTime time = [aMovie currentTime];
        [self setTime: time.timeScale * [fMovieScrubberSlider floatValue]];
        [self calculatePlaybackSMTPETimecodeForDisplay];
    }
}
#pragma mark *** Movie Notifications ***

- (void) installMovieCallbacks
{


/*Notification for any time the movie rate changes */
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(movieRateDidChange:)
                                                     name:@"QTMovieRateDidChangeNotification"
                                                   object:aMovie];
        /*Notification for when the movie ends */
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(movieDidEnd:)
                                                     name:@"QTMovieDidEndNotification"
                                                   object:aMovie];
}

- (void)removeMovieCallbacks
{
    if (aMovie)
    {
        /*Notification for any time the movie rate changes */
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:@"QTMovieRateDidChangeNotification"
                                                      object:aMovie];
        /*Notification for when the movie ends */
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:@"QTMovieDidEndNotification"
                                                      object:aMovie];
    }
}

- (void)movieRateDidChange:(NSNotification *)notification
{
    if (aMovie != nil)
    {
        /* For some stupid reason there is no "isPlaying" method for a QTMovie
         * object, given that, we detect the rate to determine whether the movie
         * is playing or not.
         */
        //[self adjustPreviewScrubberForCurrentMovieTime];
        if ([aMovie rate] != 0) // we are playing 
        {
            [fPlayPauseButton setTitle: @"||"];
        }
        else // we are paused or stopped
        {
            [fPlayPauseButton setTitle: @">"];
        }
    }
}
/* This notification is not currently used. However we should keep it "just in case" as
 * live preview playback is enhanced.
 */
- (void)movieDidEnd:(NSNotification *)notification
{

    //[fHBController writeToActivityLog: "Movie DidEnd Notification Received"];
}


#pragma mark *** QTTime Utilities ***

	// convert a time value (long) to a QTTime structure
-(void)timeToQTTime:(long)timeValue resultTime:(QTTime *)aQTTime
{
	NSNumber *timeScaleObj;
	long timeScaleValue;

	timeScaleObj = [aMovie attributeForKey:QTMovieTimeScaleAttribute];
	timeScaleValue = [timeScaleObj longValue];

	*aQTTime = QTMakeTime(timeValue, timeScaleValue);
}

	// set the movie's current time
-(void)setTime:(int)timeValue
{
	QTTime movieQTTime;
	NSValue *valueForQTTime;
	
	[self timeToQTTime:timeValue resultTime:&movieQTTime];

	valueForQTTime = [NSValue valueWithQTTime:movieQTTime];

	[aMovie setAttribute:valueForQTTime forKey:QTMovieCurrentTimeAttribute];
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
    // The min size is 480x360
    CGFloat minWidth = 480.0;
    CGFloat minHeight = 360.0;

    NSSize screenSize = [[[self window] screen] visibleFrame].size;
    NSSize sheetSize = [[self window] frame].size;
    NSSize viewAreaSize = [fPictureViewArea frame].size;
    CGFloat paddingX = 0.00;
    CGFloat paddingY = 0.00;
    
    if (fTitle->width > screenSize.width || fTitle->height > screenSize.height)
    {
        if (scaleToScreen == YES)
        {
            paddingX = screenSize.width - imageSize.width;
            paddingY = screenSize.height - imageSize.height;
        }
        
        else
        {
            paddingX = sheetSize.width - viewAreaSize.width;
            paddingY = sheetSize.height - viewAreaSize.height;  
        }

    }
    
    CGFloat maxWidth;
    CGFloat maxHeight;
    maxWidth =  screenSize.width - paddingX;
    maxHeight = screenSize.height - paddingY;
    
    NSSize resultSize = imageSize;
    CGFloat resultPar = resultSize.width / resultSize.height;

    //note, a mbp 15" at 1440 x 900 is a 1.6 ar
    CGFloat screenAspect = screenSize.width / screenSize.height;
    // Note, a standard dvd will use 720 x 480 which is a 1.5
    CGFloat viewAreaAspect = viewAreaSize.width / viewAreaSize.height;
    
    if (scaleToScreen == YES)
    {
        
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
    else if ( resultSize.width > maxWidth || resultSize.height > maxHeight )
    {
    	// Source is larger than screen in one or more dimensions
        if ( resultPar > screenAspect )
        {
            // Source aspect wider than screen aspect, snap to max width and vary height
            resultSize.width = maxWidth;
            resultSize.height = (maxWidth / resultPar);
        }
        else
        {
            // Source aspect narrower than screen aspect, snap to max height vary width
            resultSize.height = maxHeight;
            resultSize.width = (maxHeight * resultPar);
        }
    }

    // If necessary, grow to minimum dimensions to ensure controls overlay is not obstructed
    if ( resultSize.width < minWidth )
    {
        resultSize.width = minWidth;
    }
    if ( resultSize.height < minHeight )
    {
        resultSize.height = minHeight;
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
    NSSize maxSize = [[[self window] screen] visibleFrame].size;
    /* if we are not Scale To Screen, put an 85% of visible screen on the window */
    if (scaleToScreen == NO )
    {
        maxSize.width = maxSize.width * 0.85;
        maxSize.height = maxSize.height * 0.85;
    }
    
    /* Set our min size to the storage size */
    NSSize minSize;
    minSize.width = fTitle->width;
    minSize.height = fTitle->height;
    
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
    /* compare frame to max size of screen */
    
    if( frame.size.width > maxSize.width )
    {
        frame.size.width = maxSize.width;
    }
    
    if( frame.size.height > maxSize.height )
    {
        frame.size.height = maxSize.height;
    }
    
    
    

    
    // But now the sheet is off-center, so also shift the origin to center it and
    // keep the top aligned.
    if( frame.size.width != [[self window] frame].size.width )
        frame.origin.x -= (deltaX / 2.0);
    
        
        /* Since upon launch we can open up the preview window if it was open
         * the last time we quit (and at the size it was) we want to make
         * sure that upon resize we do not have the window off the screen
         * So check the origin against the screen origin and adjust if
         * necessary.
         */
        NSSize screenSize = [[[self window] screen] visibleFrame].size;
        NSPoint screenOrigin = [[[self window] screen] frame].origin;
        if (screenSize.height < frame.size.height)
        {
            frame.size.height = screenSize.height;
        }
        if (screenSize.width < frame.size.width)
        {
            frame.size.width = screenSize.width;
        }
        
        
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

//
// -[PictureController(Private) setViewSize:]
//
// Changes the view's size and centers it vertically inside of its area.
// Assumes resizeSheetForViewSize: has already been called.
//
- (void)setViewSize: (NSSize)viewSize
{   
    
    /* special case for scaleToScreen */
    NSSize screenSize = [[[self window] screen] visibleFrame].size;
    NSSize areaSize = [fPictureViewArea frame].size;
    NSSize pictureSize = [fPictureView frame].size;
    CGFloat viewSizeAspect = viewSize.width / viewSize.height;
    
    if (viewSize.width > areaSize.width || viewSize.height > areaSize.height)
    {
        
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
    NSSize newAreaSize = [fPictureViewArea frame].size;
    
    
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
