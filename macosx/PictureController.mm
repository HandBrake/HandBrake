/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "PictureController.h"

@interface PictureController (Private)

- (NSSize)optimalViewSizeForImageSize: (NSSize)imageSize;
- (void)resizeSheetForViewSize: (NSSize)viewSize;
- (void)setViewSize: (NSSize)viewSize;
- (BOOL)viewNeedsToResizeToSize: (NSSize)newSize;

@end

@implementation PictureController

- (id)initWithDelegate:(id)del
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

		delegate = del;
        fPicturePreviews = [[NSMutableDictionary dictionaryWithCapacity: HB_NUM_HBLIB_PICTURES] retain];
	}
	return self;
}

- (void) dealloc
{
    [fPicturePreviews release];
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
    [fAnamorphicPopUp selectItemAtIndex: job->pixel_ratio];
    
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
	[fVFRCheck setState:fPictureFilterSettings.vfr];
    [fDetelecineCheck setState:fPictureFilterSettings.detelecine];
    [fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];
    [fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
    [fDeblockCheck setState: fPictureFilterSettings.deblock];
    
    fPicture = 0;
    MaxOutputWidth = title->width - job->crop[2] - job->crop[3];
    MaxOutputHeight = title->height - job->crop[0] - job->crop[1];
    [self SettingsChanged: nil];
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

// Adjusts the window to draw the current picture (fPicture) adjusting its size as
// necessary to display as much of the picture as possible.
- (void) displayPreview
{
    [fPictureView setImage: [self imageForPicture: fPicture]];
    	
	NSSize displaySize = NSMakeSize( (float)fTitle->width, (float)fTitle->height );
    /* Set the picture size display fields below the Preview Picture*/
    if( fTitle->job->pixel_ratio == 1 ) // Original PAR Implementation
    {
        output_width = fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3];
        output_height = fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1];
        display_width = output_width * fTitle->job->pixel_aspect_width / fTitle->job->pixel_aspect_height;
        [fInfoField setStringValue:[NSString stringWithFormat:
                                    @"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d",
                                    fTitle->width, fTitle->height, output_width, output_height, display_width, output_height]];
        displaySize.width *= ((float)fTitle->job->pixel_aspect_width) / ((float)fTitle->job->pixel_aspect_height);   
    }
    else if (fTitle->job->pixel_ratio == 2) // Loose Anamorphic
    {
        display_width = output_width * output_par_width / output_par_height;
        [fInfoField setStringValue:[NSString stringWithFormat:
                                    @"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d",
                                    fTitle->width, fTitle->height, output_width, output_height, display_width, output_height]];
        
        /* FIXME: needs to be fixed so that the picture window does not resize itself on the first
         anamorphic width drop
         */
        if (fTitle->width - 8 < output_width)
        {
            displaySize.width *= ((float)output_par_width) / ((float)output_par_height);
        }
    }
    else // No Anamorphic
    {
        [fInfoField setStringValue: [NSString stringWithFormat:
                                     @"Source: %dx%d, Output: %dx%d", fTitle->width, fTitle->height,
                                     fTitle->job->width, fTitle->job->height]];
    }
    
    NSSize viewSize = [self optimalViewSizeForImageSize:displaySize];
    if( [self viewNeedsToResizeToSize:viewSize] )
    {
        [self resizeSheetForViewSize:viewSize];
        [self setViewSize:viewSize];
    }
    
    // Show the scaled text (use the height to check since the width can vary
    // with anamorphic video).
    if( ((int)viewSize.height) != fTitle->height )
    {
        float scale = viewSize.width / ((float)fTitle->width);
        NSString *scaleString = [NSString stringWithFormat:
                                 NSLocalizedString( @" (Preview scaled to %.0f%% actual size)",
                                                   @"String shown when a preview is scaled" ),
                                 scale * 100.0];
        [fInfoField setStringValue:
         [[fInfoField stringValue] stringByAppendingString:scaleString]];
    }
    
    [fPrevButton setEnabled: ( fPicture > 0 )];
    [fNextButton setEnabled: ( fPicture < 9 )];
}

- (IBAction) SettingsChanged: (id) sender
{
    hb_job_t * job = fTitle->job;
    
	if( [fAnamorphicPopUp indexOfSelectedItem] > 0 )
	{
        if ([fAnamorphicPopUp indexOfSelectedItem] == 2) // Loose anamorphic
        {
            job->pixel_ratio = 2;
            [fWidthStepper setEnabled: YES];
            [fWidthField setEnabled: YES];
            /* We set job->width and call hb_set_anamorphic_size in libhb to do a "dry run" to get
             * the values to be used by libhb for loose anamorphic
             */
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
            
            job->pixel_ratio = 1;
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
        job->pixel_ratio = 0;
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
    
	fPictureFilterSettings.deinterlace = [fDeinterlacePopUp indexOfSelectedItem];
    /* if the gui deinterlace settings are fast through slowest, the job->deinterlace
     value needs to be set to one, for the job as well as the previews showing deinterlacing
     otherwise set job->deinterlace to 0 or "off" */
    if (fPictureFilterSettings.deinterlace > 0)
    {
        job->deinterlace  = 1;
    }
    else
    {
        job->deinterlace  = 0;
    }
    fPictureFilterSettings.denoise     = [fDenoisePopUp indexOfSelectedItem];
    fPictureFilterSettings.vfr  = [fVFRCheck state];
    if (fPictureFilterSettings.vfr > 0)
    {
        [fDetelecineCheck setState:NSOnState];
        [fDetelecineCheck setEnabled: NO];
    }
    else
    {
        [fDetelecineCheck setEnabled: YES];
    }
    fPictureFilterSettings.detelecine  = [fDetelecineCheck state];
    fPictureFilterSettings.deblock  = [fDeblockCheck state];
	//job->pixel_ratio = ( [fPARCheck state] == NSOnState );
    
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
    /* Sanity Check Here for < 16 px preview to avoid
     crashing hb_get_preview. In fact, just for kicks
     lets getting previews at a min limit of 32, since
     no human can see any meaningful detail below that */
    if (job->width >= 64 && job->height >= 64)
    {
        // Purge the existing picture previews so they get recreated the next time
        // they are needed.
        [self purgeImageCache];
        [self displayPreview];
    }
}

- (IBAction) PreviousPicture: (id) sender
{   
    if( fPicture <= 0 )
    {
        return;
    }
    fPicture--;
    [self displayPreview];
}

- (IBAction) NextPicture: (id) sender
{
    if( fPicture >= 9 )
    {
        return;
    }
    fPicture++;
    [self displayPreview];
}

- (IBAction) ClosePanel: (id) sender
{
    if ([delegate respondsToSelector:@selector(pictureSettingsDidChange)])
        [delegate pictureSettingsDidChange];

    [NSApp endSheet:[self window]];
    [[self window] orderOut:self];
}

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

- (int) vfr
{
    return fPictureFilterSettings.vfr;
}

- (void) setVFR: (int) setting
{
    fPictureFilterSettings.vfr = setting;
}

- (int) deinterlace
{
    return fPictureFilterSettings.deinterlace;
}

- (void) setDeinterlace: (int) setting {
    fPictureFilterSettings.deinterlace = setting;
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

- (void)showPanelInWindow: (NSWindow *)fWindow forTitle: (hb_title_t *)title
{
    [self SetTitle:title];

    [NSApp beginSheet:[self window]
       modalForWindow:fWindow
        modalDelegate:nil
       didEndSelector:nil
          contextInfo:NULL];
}


// This function converts an image created by libhb (specified via pictureIndex) into
// an NSImage suitable for the GUI code to use. If removeBorders is YES,
// makeImageForPicture crops the image generated by libhb stripping off the gray
// border around the content. This is the low-level method that generates the image.
// -imageForPicture calls this function whenever it can't find an image in its cache.
+ (NSImage *) makeImageForPicture: (int)pictureIndex
                libhb:(hb_handle_t*)handle
                title:(hb_title_t*)title
                removeBorders:(BOOL)removeBorders
{
    if (removeBorders)
    {
        //     |<---------- title->width ----------->|
        //     |   |<---- title->job->width ---->|   |
        //     |   |                             |   |
        //     .......................................
        //     ....+-----------------------------+....
        //     ....|                             |....<-- gray border
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |<------- image
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |....
        //     ....+-----------------------------+....
        //     .......................................

        static uint8_t * buffer;
        static int bufferSize;

        // Make sure we have a big enough buffer to receive the image from libhb. libhb
        // creates images with a one-pixel border around the original content. Hence we
        // add 2 pixels horizontally and vertically to the buffer size.
        int srcWidth = title->width + 2;
        int srcHeight= title->height + 2;
        int newSize;
        newSize = srcWidth * srcHeight * 4;
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
        
        int dstWidth = title->job->width;
        int dstHeight = title->job->height;
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

        int borderTop = (srcHeight - dstHeight) / 2;
        int borderLeft = (srcWidth - dstWidth) / 2;
        
        UInt32 * src = (UInt32 *)buffer;
        UInt32 * dst = (UInt32 *)[imgrep bitmapData];
        src += borderTop * srcWidth;    // skip top rows in src to get to first row of dst
        src += borderLeft;              // skip left pixels in src to get to first pixel of dst
        for (int r = 0; r < dstHeight; r++)
        {
            for (int c = 0; c < dstWidth; c++)
#if TARGET_RT_LITTLE_ENDIAN
                *dst++ = Endian32_Swap(*src++);
#else
                *dst++ = *src++;
#endif
            src += (srcWidth - dstWidth);   // skip to next row in src
        }

        NSImage * img = [[[NSImage alloc] initWithSize: NSMakeSize(dstWidth, dstHeight)] autorelease];
        [img addRepresentation:imgrep];

        return img;
    }
    else
    {
        // Make sure we have big enough buffer
        static uint8_t * buffer;
        static int bufferSize;

        int newSize;
        newSize = ( title->width + 2 ) * (title->height + 2 ) * 4;
        if( bufferSize < newSize )
        {
            bufferSize = newSize;
            buffer     = (uint8_t *) realloc( buffer, bufferSize );
        }

        hb_get_preview( handle, title, pictureIndex, buffer );

        // The image data returned by hb_get_preview is 4 bytes per pixel, BGRA format.
        // We'll copy that into an NSImage swapping it to ARGB in the process. Alpha is
        // ignored.
        int width = title->width + 2;      // hblib adds a one-pixel border to the image
        int height = title->height + 2;
        int numPixels = width * height;
        NSBitmapFormat bitmapFormat = (NSBitmapFormat)NSAlphaFirstBitmapFormat;
        NSBitmapImageRep * imgrep = [[[NSBitmapImageRep alloc]
                initWithBitmapDataPlanes:nil
                pixelsWide:width
                pixelsHigh:height
                bitsPerSample:8
                samplesPerPixel:3   // ignore alpha
                hasAlpha:NO
                isPlanar:NO
                colorSpaceName:NSCalibratedRGBColorSpace
                bitmapFormat:bitmapFormat
                bytesPerRow:width * 4
                bitsPerPixel:32] autorelease];

        UInt32 * src = (UInt32 *)buffer;
        UInt32 * dst = (UInt32 *)[imgrep bitmapData];
        for (int i = 0; i < numPixels; i++)
#if TARGET_RT_LITTLE_ENDIAN
            *dst++ = Endian32_Swap(*src++);
#else
            *dst++ = *src++;
#endif

        NSImage * img = [[[NSImage alloc] initWithSize: NSMakeSize(width, height)] autorelease];
        [img addRepresentation:imgrep];

        return img;
    }
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
        theImage = [PictureController makeImageForPicture:pictureIndex libhb:fHandle title:fTitle removeBorders: NO];
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

@end

@implementation PictureController (Private)

//
// -[PictureController(Private) optimalViewSizeForImageSize:]
//
// Given the size of the preview image to be shown, returns the best possible
// size for the OpenGL view.
//
- (NSSize)optimalViewSizeForImageSize: (NSSize)imageSize
{
    // The min size is 320x240
    float minWidth = 320.0;
    float minHeight = 240.0;

    // The max size of the view is when the sheet is taking up 85% of the screen.
    NSSize screenSize = [[NSScreen mainScreen] frame].size;
    NSSize sheetSize = [[self window] frame].size;
    NSSize viewAreaSize = [fPictureViewArea frame].size;
    float paddingX = sheetSize.width - viewAreaSize.width;
    float paddingY = sheetSize.height - viewAreaSize.height;
    float maxWidth = (0.85 * screenSize.width) - paddingX;
    float maxHeight = (0.85 * screenSize.height) - paddingY;
    
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
    
    return resultSize;
}

//
// -[PictureController(Private) resizePanelForViewSize:animate:]
//
// Resizes the entire sheet to accomodate an OpenGL view of a particular size.
//
- (void)resizeSheetForViewSize: (NSSize)viewSize
{
    // Figure out the deltas for the new frame area
    NSSize currentSize = [fPictureViewArea frame].size;
    float deltaX = viewSize.width - currentSize.width;
    float deltaY = viewSize.height - currentSize.height;

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
    frame.origin.x -= (deltaX / 2.0);
    frame.origin.y -= deltaY;

    [[self window] setFrame:frame display:YES animate:YES];
}

//
// -[PictureController(Private) setViewSize:]
//
// Changes the OpenGL view's size and centers it vertially inside of its area.
// Assumes resizeSheetForViewSize: has already been called.
//
- (void)setViewSize: (NSSize)viewSize
{
    [fPictureView setFrameSize:viewSize];
    
    // center it vertically
    NSPoint origin = [fPictureViewArea frame].origin;
    origin.y += ([fPictureViewArea frame].size.height -
                 [fPictureView frame].size.height) / 2.0;
    [fPictureView setFrameOrigin:origin];
}

//
// -[PictureController(Private) viewNeedsToResizeToSize:]
//
// Returns YES if the view will need to resize to match the given size.
//
- (BOOL)viewNeedsToResizeToSize: (NSSize)newSize
{
    NSSize viewSize = [fPictureView frame].size;
    return (newSize.width != viewSize.width || newSize.height != viewSize.height);
}


@end
