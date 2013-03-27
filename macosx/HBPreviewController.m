/* $Id: HBPreviewController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBPreviewController.h"
#import "Controller.h"

#define BORDER_SIZE 2.0
#define HB_NUM_HBLIB_PICTURES 20   // # of preview pictures libhb should generate

@implementation QTMovieView (HBExtensions)
- (void) mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];
}
@end

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
@interface NSWindow(HBExtensions)

@property (readonly) CGFloat backingScaleFactor;

@end
#endif

@interface PreviewController (Private)

- (NSSize)optimalViewSizeForImageSize: (NSSize)imageSize;
- (void)resizeWindowForViewSize: (NSSize)viewSize;
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

- (void)setHBController: (HBController *)controller
{
    fHBController = controller;
}

- (void)awakeFromNib
{
    [[self window] setDelegate:self];

    if( ![[self window] setFrameUsingName:@"Preview"] )
        [[self window] center];

    [self setWindowFrameAutosaveName:@"Preview"];
    [[self window] setExcludedFromWindowsMenu:YES];
    
    /* lets set the preview window to accept mouse moved events */
    [[self window] setAcceptsMouseMovedEvents:YES];
    [self startReceivingLibhbNotifications];
    
    hudTimerSeconds = 0;
    /* we set the progress indicator to not use threaded animation
     * as it causes a conflict with the qtmovieview's controllerbar
    */
    [fMovieCreationProgressIndicator setUsesThreadedAnimation:NO];
    
    /* we set the preview length popup in seconds */
    [fPreviewMovieLengthPopUp removeAllItems];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"15"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"30"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"45"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"60"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"90"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"105"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"120"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"135"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"150"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"165"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"180"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"195"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"210"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"225"];
    [fPreviewMovieLengthPopUp addItemWithTitle: @"240"];

	[fMovieView setHidden:YES];
    [fMovieView setDelegate:self];

    /* Setup our layers for core animation */
    [[[self window] contentView] setWantsLayer:YES];
    [fPictureControlBox setWantsLayer:YES];
    [fEncodingControlBox setWantsLayer:YES];
    [fMoviePlaybackControlBox setWantsLayer:YES];
    
    fWhiteBackground = [CALayer layer];
    [fWhiteBackground setBounds:CGRectMake(0.0, 0.0, 480.0, 360.0)];
    [fWhiteBackground setPosition:CGPointMake([[[self window] contentView] frame].size.width /2,
                                 [[[self window] contentView] frame].size.height /2)];
    
    [fWhiteBackground setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
    CGColorRef white = CGColorCreateGenericRGB(1.0, 1.0, 1.0, 1.0);
    [fWhiteBackground setBackgroundColor: white];
    CFRelease(white);
    [fWhiteBackground setShadowOpacity:0.5f];
    [fWhiteBackground setShadowOffset:CGSizeMake(0, 0)];

    fPictureLayer = [CALayer layer];
    [fPictureLayer setBounds:CGRectMake(0.0, 0.0, 476.0, 356.0)];
    [fPictureLayer setPosition:CGPointMake([[[self window] contentView] frame].size.width /2,
                                             [[[self window] contentView] frame].size.height /2)];
    
    [fPictureLayer setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];

    NSMutableDictionary *actions = [NSMutableDictionary
                                    dictionaryWithDictionary:[fPictureLayer actions]];

    // Disable fade on contents change
    [actions setObject:[NSNull null] forKey:@"contents"];
    [fPictureLayer setActions:actions];

    [[[[self window] contentView] layer] insertSublayer:fWhiteBackground below: [fMovieView layer]];
    [[[[self window] contentView] layer] insertSublayer:fPictureLayer below: [fMovieView layer]];

    /* relocate our hud origins */
    NSPoint hudControlBoxOrigin = [fMoviePlaybackControlBox frame].origin;
    [fPictureControlBox setFrameOrigin:hudControlBoxOrigin];
    [fEncodingControlBox setFrameOrigin:hudControlBoxOrigin];
    [fMoviePlaybackControlBox setFrameOrigin:hudControlBoxOrigin];
    
    if( [[self window] respondsToSelector:@selector( backingScaleFactor )] )
        backingScaleFactor = [[self window] backingScaleFactor];
    else
        backingScaleFactor = 1.0;
    
    [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(titleChanged:) name: HBTitleChangedNotification object:nil];
}

- (void) titleChanged: (NSNotification *) aNotification
{
    /* Notification from HBController, only used to stop
     * an encoding while the HBController is scanning a new title
     */
    [self cancelCreateMoviePreview:self];
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    /* Upon closing the preview window, we make sure we clean up any
     * preview movie that might be playing or encoding. However, first
     * make sure we have a preview picture before calling pictureSliderChanged
     * to go back to still previews .. just in case nothing is loaded up like in
     * a Launch, cancel new scan then quit type scenario.
     */

    if (fEncodeState || [self isPlaying])
    {
        [self cancelCreateMoviePreview:self];
        [fMovieView pause:self];
        [self stopMovieTimer];
    }

    hudTimerSeconds = 0;
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PreviewWindowIsOpen"];
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
    
    hb_close(&fPreviewLibhb);
    
    [self removeMovieCallbacks];
    
    [super dealloc];
}

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle = handle;
    
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
        /* currently hard set default to 15 seconds */
        [fPreviewMovieLengthPopUp selectItemAtIndex: 0];
    }
}

- (void) SetTitle: (hb_title_t *) title
{
    fTitle = title;
    fPicture = 0;

    [self settingsChanged:nil];
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification {
    
    NSWindow *theWindow = (NSWindow *)[notification object];
    
    CGFloat newBackingScaleFactor = [theWindow backingScaleFactor];
    CGFloat oldBackingScaleFactor = [[[notification userInfo]
                                      objectForKey:@"NSBackingPropertyOldScaleFactorKey"]
                                     doubleValue];
    if( newBackingScaleFactor != oldBackingScaleFactor )
    {
        // Scale factor changed, update the preview window
        // to the new situation
        backingScaleFactor = newBackingScaleFactor;
        if (fTitle)
            [self pictureSliderChanged:self];
    }
}

// Adjusts the window to draw the current picture (fPicture) adjusting its size as
// necessary to display as much of the picture as possible.
- (void) displayPreview
{
    hb_job_t * job = fTitle->job;

    NSImage *fPreviewImage = [self imageForPicture: fPicture];
    NSSize imageScaledSize = [fPreviewImage size];
    [fPictureLayer setContents:fPreviewImage];

    NSSize displaySize = NSMakeSize( ( CGFloat )fTitle->width, ( CGFloat )fTitle->height );
    NSString *sizeInfoString;

    /* Set the picture size display fields below the Preview Picture*/
    int output_width, output_height, output_par_width, output_par_height;
    int display_width;
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

    if( backingScaleFactor != 1.0 )
    {
        // HiDPI mode usually display everything
        // with douple pixel count, but we don't
        // want to double the size of the video
        displaySize.height /= backingScaleFactor;
        displaySize.width /= backingScaleFactor;
        imageScaledSize.height /= backingScaleFactor;
        imageScaledSize.width /= backingScaleFactor;
    }

    // Get the optimal view size for the image
    NSSize viewSize = [self optimalViewSizeForImageSize:displaySize];
    viewSize.width += BORDER_SIZE * 2;
    viewSize.height += BORDER_SIZE * 2;
    
    NSSize windowSize;
    if (scaleToScreen == YES)
        // Scale the window to the max possible size
        windowSize = [[[self window] screen] visibleFrame].size;
    else
        // Scale the window to the image size
        windowSize = viewSize;

    [self resizeWindowForViewSize:windowSize];
    NSSize areaSize = [[[self window] contentView] frame].size;
    areaSize.width -= BORDER_SIZE * 2;
    areaSize.height -= BORDER_SIZE * 2;

    if (scaleToScreen == YES)
    {
        /* We are in Scale To Screen mode so, we have to get the ratio for height and width against the window
         *size so we can scale from there.
         */
        CGFloat pictureAspectRatio = imageScaledSize.width / imageScaledSize.height;
        CGFloat areaAspectRatio = areaSize.width / areaSize.height;
        
        if (pictureAspectRatio > areaAspectRatio)
        {
            viewSize.width = areaSize.width;
            viewSize.height = viewSize.width / pictureAspectRatio;
        }
        else
        {
            viewSize.height = areaSize.height;
            viewSize.width = viewSize.height * pictureAspectRatio;
        }
    }
    else
    {
        // If the image is larger then the window, scale the image
        viewSize = imageScaledSize;

        if (imageScaledSize.width > areaSize.width || imageScaledSize.height > areaSize.height)
        {
            CGFloat pictureAspectRatio = imageScaledSize.width / imageScaledSize.height;
            CGFloat areaAspectRatio = areaSize.width / areaSize.height;
            
            if (pictureAspectRatio > areaAspectRatio)
            {
                viewSize.width = areaSize.width;
                viewSize.height = viewSize.width / pictureAspectRatio;
            }
            else
            {
                viewSize.height = areaSize.height;
                viewSize.width = viewSize.height * pictureAspectRatio;
            }
        }
    }

    // Resize the CALayers
    [fWhiteBackground setBounds:CGRectMake(0, 0, viewSize.width + (BORDER_SIZE * 2), viewSize.height + (BORDER_SIZE * 2))];
    [fPictureLayer setBounds:CGRectMake(0, 0, viewSize.width, viewSize.height)];

    NSString *scaleString;
    CGFloat scale = ( ( CGFloat )[fPictureLayer frame].size.width) / ( ( CGFloat )imageScaledSize.width);
    if (scale * 100.0 != 100)
        scaleString = [NSString stringWithFormat:
                       NSLocalizedString( @" (%.0f%% actual size)",
                                         @"String shown when a preview is scaled" ), scale * 100.0];
    else
        scaleString = @"(Actual size)";
    
    if (scaleToScreen == YES)
        scaleString = [scaleString stringByAppendingString:@" Scaled To Screen"];

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
    
- (IBAction) settingsChanged: (id) sender
{
    // Purge the existing picture previews so they get recreated the next time
    // they are needed.
    [self purgeImageCache];
    [self pictureSliderChanged:nil];
}

- (IBAction) pictureSliderChanged: (id) sender
{
    /* Run cancelCreateMoviePreview in case a preview is being encoded and then cancel if so */
    [self cancelCreateMoviePreview:nil];
    
    // Show the picture view
    if (aMovie)
    {
        [fMoviePlaybackControlBox setHidden:YES];
        [fMovieView pause:nil];
        [fMovieView setHidden:YES];
        [fMovieView setMovie:nil];
        aMovie = nil;
    }
    
    int newPicture = [fPictureSlider intValue];
    if (newPicture != fPicture)
    {
        fPicture = newPicture;
    }
    [self displayPreview];
    
}

- (IBAction)showWindow:(id)sender
{
    if (aMovie)
        [self startMovieTimer];
    
    [super showWindow:sender];
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
/* enableHudControls and disableHudControls are used to sync enableUI
 * in HBController so that during a scan we do not attempt to access source
 * images, etc. which can cause a crash. In general this ui behavior will mirror
 * the main window ui's enableUI method and in fact is called from there */
- (void) enableHudControls
{
    [fPictureSlider setEnabled:YES];
    [fScaleToScreenToggleButton setEnabled:YES];
    [fCreatePreviewMovieButton setEnabled:YES];
    [fGoToStillPreviewButton setEnabled:YES];
}

- (void) disableHudControls
{
    [fPictureSlider setEnabled:NO];
    [fScaleToScreenToggleButton setEnabled:NO];
    [fCreatePreviewMovieButton setEnabled:NO];
    [fGoToStillPreviewButton setEnabled:NO];
}

- (void) mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];
    NSPoint mouseLoc = [theEvent locationInWindow];
    
    /* Test for mouse location to show/hide hud controls */
    if( fEncodeState != 1 )
    {
        /* Since we are not encoding, verify which control hud to show
         * or hide based on aMovie ( aMovie indicates we need movie controls )
         */
        NSBox *hudBoxToShow;
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
		else if( NSPointInRect( mouseLoc, [[[self window] contentView] frame] ) )
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

#pragma mark Still Preview Image Processing

// This function converts an image created by libhb (specified via pictureIndex) into
// an NSImage suitable for the GUI code to use. If removeBorders is YES,
// makeImageForPicture crops the image generated by libhb stripping off the gray
// border around the content. This is the low-level method that generates the image.
// -imageForPicture calls this function whenever it can't find an image in its cache.
+ (NSImage *) makeImageForPicture: (NSInteger)pictureIndex
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

    hb_get_preview( handle, title->job, (int)pictureIndex, buffer );

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
- (NSImage *) imageForPicture: (NSInteger) pictureIndex
{
    // The preview for the specified index may not currently exist, so this method
    // generates it if necessary.
    NSNumber * key = [NSNumber numberWithInteger:pictureIndex];
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
    hb_get_state2(fPreviewLibhb, &s);
    
    if (fEncodeState && (s.state == HB_STATE_WORKING ||
                         s.state == HB_STATE_PAUSED))
    {
        fEncodeState = 2;
        hb_stop(fPreviewLibhb);
        hb_system_sleep_allow(fPreviewLibhb);
        [NSAnimationContext beginGrouping];
        [[NSAnimationContext currentContext] setDuration:0.2];
        [[fEncodingControlBox animator] setHidden:YES];
        [[fPictureControlBox animator] setHidden:NO];
        [NSAnimationContext endGrouping];

        return;
    }
}

- (IBAction) createMoviePreview: (id) sender
{    
    /* Rip or Cancel ? */
    hb_state_t s;
    hb_get_state2( fPreviewLibhb, &s );
    
    /* we use controller.mm's prepareJobForPreview to go ahead and set all of our settings
     * however, we want to use a temporary destination field of course
     * so that we do not put our temp preview in the users chosen
     * directory */
    
    hb_job_t * job = fTitle->job;
    
    /* We run our current setting through prepeareJob in Controller.mm
     * just as if it were a regular encode */
    
    [fHBController prepareJobForPreview];
    
    /* Make sure we have a Preview sub directory with our pidnum attached */
    NSString *PreviewDirectory = [NSString stringWithFormat:@"~/Library/Application Support/HandBrake/Previews/%d", [fHBController getPidnum]];
    PreviewDirectory = [PreviewDirectory stringByExpandingTildeInPath];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:PreviewDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:PreviewDirectory 
                                  withIntermediateDirectories:NO 
                                                   attributes:nil 
                                                        error:nil];
    }
    /* Destination file. We set this to our preview directory
     * changing the extension appropriately.*/
    if (fTitle->job->mux == HB_MUX_MP4) // MP4 file
    {
        /* we use .m4v for our mp4 files so that ac3 and chapters in mp4 will play properly */
        fPreviewMoviePath = [PreviewDirectory stringByAppendingString:@"/preview_temp.m4v"];
    }
    else if (fTitle->job->mux == HB_MUX_MKV) // MKV file
    {
        fPreviewMoviePath = [PreviewDirectory stringByAppendingString:@"/preview_temp.mkv"];
    }
    
    fPreviewMoviePath = [[fPreviewMoviePath stringByExpandingTildeInPath]retain];
    
    /* See if there is an existing preview file, if so, delete it */
    if( ![[NSFileManager defaultManager] fileExistsAtPath:fPreviewMoviePath] )
    {
        [[NSFileManager defaultManager] removeItemAtPath:fPreviewMoviePath error:nil];
    }
    
    /* We now direct our preview encode to fPreviewMoviePath */
    hb_job_set_file(fTitle->job, [fPreviewMoviePath UTF8String]);
    
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
        x264opts_tmp = job->advanced_opts;
        
        job->advanced_opts = NULL;
        job->indepth_scan = 1;  
        /*
         * Add the pre-scan job
         */
        hb_add( fPreviewLibhb, job );
        job->advanced_opts = x264opts_tmp;
    }                  
    /* Go ahead and perform the actual encoding preview scan */
    job->indepth_scan = 0;
    job->pass = 0;
    hb_add( fPreviewLibhb, job );

    /* we need to clean up the various lists after the job(s) have been set  */
    hb_job_reset( job );

    [fMovieCreationProgressIndicator setDoubleValue:0];

    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.2];
    [[fEncodingControlBox animator] setHidden: NO];
    [[fPictureControlBox animator] setHidden: YES];
    [NSAnimationContext endGrouping];

    /* Let fPreviewLibhb do the job */
    fEncodeState = 1;
    hb_system_sleep_prevent(fPreviewLibhb);
    hb_start(fPreviewLibhb);
	
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
            
            break;
            
        }
#undef p
            
#define p state.param.muxing
        case HB_STATE_MUXING:
        {
            // Update fMovieCreationProgressIndicator
            [fMovieCreationProgressIndicator setIndeterminate: YES];
            [fMovieCreationProgressIndicator startAnimation: nil];
            [fPreviewMovieStatusField setStringValue: NSLocalizedString( @"Muxing Preview ...", @"" )];
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
            [fMovieCreationProgressIndicator stopAnimation: nil];

            if (fEncodeState != 2)
            {
                // Show the movie view
                [self showMoviePreview:fPreviewMoviePath];
            }
            
            fEncodeState = 0;
            /* Done encoding, allow system sleep for the preview handle */
            hb_system_sleep_allow(fPreviewLibhb);
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
        if ([self isPlaying]) // we are playing
        {
            [fMovieView pause:aMovie];
            [fPlayPauseButton setState: NSOnState];
        }
        else // we are paused or stopped
        {
            [fMovieView play:aMovie];
            [fPlayPauseButton setState: NSOffState];
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
        [fMovieInfoField setStringValue: [self SMTPETimecode:[aMovie currentTime]]];
    }
}

- (IBAction) showPicturesPreview: (id) sender
{
    [fMovieView pause:self];
    [self stopMovieTimer];

    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.2];
    [[fMoviePlaybackControlBox animator] setHidden:YES];
    [[fMovieView animator] setHidden:YES];
    [[fPictureControlBox animator] setHidden:NO];
    [NSAnimationContext endGrouping];

    [fMovieView setMovie:nil];
    aMovie = nil;
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
            [fHBController writeToActivityLog: "showMoviePreview: Unable to open movie"];
		}
        else 
        {
            NSRect movieBounds;

            [fMovieView setControllerVisible:NO];

            /* we get some size information from the preview movie */
            NSSize movieSize = [[aMovie attributeForKey:QTMovieNaturalSizeAttribute] sizeValue];
            movieBounds = [fMovieView movieBounds];
            movieBounds.size.height = movieSize.height;
            /* We also get our view size to use for scaling fMovieView's size */
            NSSize scaledMovieViewSize = [fPictureLayer frame].size;
            movieBounds.size.width = movieSize.width;
            
            /* we need to account for an issue where the scaledMovieViewSize > the window size */
            if (scaledMovieViewSize.height > [[self window] frame].size.height)
            {
                [fHBController writeToActivityLog: "showMoviePreview: Our window is not tall enough to show the controller bar ..."];
            }
            
            /* Scale the fMovieView to scaledMovieViewSize */
            [fMovieView setFrameSize:scaledMovieViewSize];
            
            /*set our origin try using fPictureViewArea or fPictureView */
            NSPoint origin = [fPictureLayer frame].origin;
            origin.x += trunc( ( [fPictureLayer frame].size.width -
                                [fMovieView frame].size.width ) / 2.0 );
            origin.y += trunc( ( ( [fPictureLayer frame].size.height -
                                      [fMovieView frame].size.height ) / 2.0 ) );

            [fMovieView setFrameOrigin:origin];
            [fMovieView setMovie:aMovie];
            
            // get and enable subtitles
            NSArray *subtitlesArray;
            subtitlesArray = [aMovie tracksOfMediaType: @"sbtl"];
            if( subtitlesArray && [subtitlesArray count] )
            {
                // enable the first TX3G subtitle track
                [[subtitlesArray objectAtIndex: 0] setEnabled: YES];
            }
            else
            {
                // Perian subtitles
                subtitlesArray = [aMovie tracksOfMediaType: QTMediaTypeVideo];
                if( subtitlesArray && ( [subtitlesArray count] >= 2 ) )
                {
                    // track 0 should be video, other video tracks should
                    // be subtitles; force-enable the first subs track
                    [[subtitlesArray objectAtIndex: 1] setEnabled: YES];
                }
            }
            
            // to actually play the movie
            [NSAnimationContext beginGrouping];
            [[NSAnimationContext currentContext] setDuration:0.2];
            [[fEncodingControlBox animator] setHidden: YES];
            [[fMovieView animator] setHidden:NO];
            [[fMoviePlaybackControlBox animator] setHidden: NO];
            [NSAnimationContext endGrouping];

            [self initPreviewScrubberForMovie];
            [self startMovieTimer];
            /* Install amovie notifications */
            [aMovie setDelegate:self];
            [self installMovieCallbacks];
            [fMovieView play:aMovie];
        }
    }
}

#pragma mark *** Movie Playback Scrubber and time code methods ***

// Initialize the preview scrubber min/max to appropriate values for the current movie
-(void) initPreviewScrubberForMovie
{
    if (aMovie)
    {
        QTTime duration = [aMovie duration];
        CGFloat result = duration.timeValue / duration.timeScale;
        
        [fMovieScrubberSlider setMinValue:0.0];
        [fMovieScrubberSlider setMaxValue: result];
        [fMovieScrubberSlider setDoubleValue: 0.0];
    }
}

-(void) adjustPreviewScrubberForCurrentMovieTime
{
    if (aMovie)
    {
        QTTime time = [aMovie currentTime];
        
        CGFloat result = (CGFloat)time.timeValue / (CGFloat)time.timeScale;;
        [fMovieScrubberSlider setDoubleValue:result];
    }
}

- (IBAction) previewScrubberChanged: (id) sender
{
    if (aMovie)
    {
        [fMovieView pause:aMovie];
        QTTime time = [self SliderToQTTime:[fMovieScrubberSlider doubleValue]];
        [aMovie setCurrentTime:time];
        [fMovieInfoField setStringValue: [self SMTPETimecode:time]];
    }
}

- (BOOL) isPlaying
{
    if (aMovie != nil)
    {
        /* For some stupid reason there is no "isPlaying" method for a QTMovie
         * object, given that, we detect the rate to determine whether the movie
         * is playing or not.
         */
        if ([aMovie rate] != 0.0f) // we are playing
            return YES;
        else // we are paused or stopped
            return NO;
    }
    return NO;
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
        if ([self isPlaying])
            [fPlayPauseButton setState: NSOnState];
        else
            [fPlayPauseButton setState: NSOffState];
    }
}

/* This notification is not currently used. However we should keep it "just in case" as
 * live preview playback is enhanced.
 */
- (void)movieDidEnd:(NSNotification *)notification
{
    //[fHBController writeToActivityLog: "Movie DidEnd Notification Received"];
}

/* fMovieView Keyboard controls */
- (void)keyDown:(NSEvent *)event
{
    unichar key = [[event charactersIgnoringModifiers] characterAtIndex:0];

    if (aMovie)
    {
        if (key == 32)
        {
            if ([self isPlaying])
                [fMovieView pause:aMovie];
            else
                [fMovieView play:aMovie];
        }
        else if (key == 'k')
            [fMovieView pause:aMovie];
        else if (key == 'l')
        {
            float rate = [aMovie rate];
            rate += 1.0f;
            [fMovieView play:aMovie];
            [aMovie setRate:rate];
        }
        else if (key == 'j')
        {
            float rate = [aMovie rate];
            rate -= 1.0f;
            [fMovieView play:aMovie];
            [aMovie setRate:rate];
        }
        else if ([event modifierFlags] & NSAlternateKeyMask && key == NSLeftArrowFunctionKey)
            [fMovieView gotoBeginning:self];
        else if ([event modifierFlags] & NSAlternateKeyMask && key == NSRightArrowFunctionKey)
            [fMovieView gotoEnd:self];
        else if (key == NSLeftArrowFunctionKey)
            [fMovieView stepBackward:self];
        else if (key == NSRightArrowFunctionKey)
            [fMovieView stepForward:self];
        else
            [super keyDown:event];
    }
    else if (!fEncodeState)
    {
        if (key == NSLeftArrowFunctionKey)
        {
            [fPictureSlider setIntegerValue:fPicture > [fPictureSlider minValue] ? fPicture - 1 : fPicture];
            [self pictureSliderChanged:self];
        }
        else if (key == NSRightArrowFunctionKey)
        {
            [fPictureSlider setIntegerValue:fPicture < [fPictureSlider maxValue] ? fPicture + 1 : fPicture];
            [self pictureSliderChanged:self];
        }
        else
            [super keyDown:event];
    }
    else
        [super keyDown:event];
}

#pragma mark *** QTTime Utilities ***

	// convert a time value (long) to a QTTime structure
-(QTTime)SliderToQTTime:(double)value
{
	long timeScale = [[aMovie attributeForKey:QTMovieTimeScaleAttribute] longValue];
	return QTMakeTime(value * timeScale, timeScale);
}

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
- (NSString*) SMTPETimecode:(QTTime)time
{
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
    
    smtpeTimeCodeString = [NSString stringWithFormat:@"%02d:%02d:%02d", hour, minute, second]; // hh:mm:ss
    return smtpeTimeCodeString;
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
    CGFloat maxWidth = screenSize.width;
    CGFloat maxHeight = screenSize.height;

    NSSize resultSize = imageSize;
    CGFloat resultPar = resultSize.width / resultSize.height;

    //note, a mbp 15" at 1440 x 900 is a 1.6 ar
    CGFloat screenAspect = screenSize.width / screenSize.height;

    if ( resultSize.width > maxWidth || resultSize.height > maxHeight )
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
        resultSize.width = minWidth;
    if ( resultSize.height < minHeight )
        resultSize.height = minHeight;

    return resultSize;
}

//
// -[PictureController(Private) resizeWindowForViewSize:]
//
// Resizes the entire window to accomodate a view of a particular size.
//
- (void)resizeWindowForViewSize: (NSSize)viewSize
{
    // Figure out the deltas for the new frame area
    NSSize currentSize = [[[self window] contentView] frame].size;
    CGFloat deltaX = viewSize.width - currentSize.width;
    CGFloat deltaY = viewSize.height - currentSize.height;
    
    // Now resize the whole panel by those same deltas, but don't exceed the min
    NSRect frame = [[self window] frame];
    NSSize maxSize = [[[self window] screen] visibleFrame].size;
    /* if we are not Scale To Screen, put an 10% of visible screen on the window */
    if (scaleToScreen == NO )
    {
        maxSize.width = maxSize.width * 0.90;
        maxSize.height = maxSize.height * 0.90;
    }
    
    /* Set our min size to the storage size */
    NSSize minSize;
    minSize.width = fTitle->width / backingScaleFactor;
    minSize.height = fTitle->height / backingScaleFactor;
    
    frame.size.width += deltaX;
    frame.size.height += deltaY;
    if( frame.size.width < minSize.width )
    {
        frame.size.width = minSize.width;
        deltaX = frame.size.width - currentSize.width;
    }
    if( frame.size.height < minSize.height )
    {
        frame.size.height = minSize.height;
        deltaY = frame.size.height - currentSize.height;
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

@end
