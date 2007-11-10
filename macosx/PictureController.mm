/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "PictureController.h"

@interface PictureController (Private)

- (NSSize)optimalViewSizeForImageSize: (NSSize)imageSize;
- (void)resizeSheetForViewSize: (NSSize)viewSize;
- (void)setViewSize: (NSSize)viewSize;
- (BOOL)viewNeedsToResizeToSize: (NSSize)newSize;

@end

static int GetAlignedSize( int size )
{
    int result = 1;
    while( result < size )
    {
        result *= 2;
    }
    return result;
}

@implementation PictureController

- (id)initWithDelegate:(id)del
{
	if (self = [super init])
	{
		delegate = del;
        [self loadMyNibFile];
	}
	return self;
}

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle = handle;

    fHasQE = CGDisplayUsesOpenGLAcceleration( kCGDirectMainDisplay );

    fBuffer     = NULL;
    fBufferSize = 0;
    fTexBuf[0]  = NULL;
    fTexBuf[1]  = NULL;
    fTexBufSize = 0;

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

    /* Make sure we have big enough buffers */
    int newSize;
    newSize = ( title->width + 2 ) * (title->height + 2 ) * 4;
    if( fBufferSize < newSize )
    {
        fBufferSize = newSize;
        fBuffer     = (uint8_t *) realloc( fBuffer, fBufferSize );
    }
    if( !fHasQE )
    {
        newSize = ( GetAlignedSize( title->width + 2 ) *
            GetAlignedSize( title->height + 2 ) * 4 );
    }
    if( fTexBufSize < newSize )
    {
        fTexBufSize = newSize;
        fTexBuf[0]  = (uint8_t *) realloc( fTexBuf[0], fTexBufSize );
        fTexBuf[1]  = (uint8_t *) realloc( fTexBuf[1], fTexBufSize );
    }


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
    
	
	/* we use a popup to show the deinterlace settings */
	[fDeinterlacePopUp removeAllItems];
    [fDeinterlacePopUp addItemWithTitle: @"None"];
    [fDeinterlacePopUp addItemWithTitle: @"Fast"];
    [fDeinterlacePopUp addItemWithTitle: @"Slow"];
	[fDeinterlacePopUp addItemWithTitle: @"Slower"];
	[fDeinterlacePopUp addItemWithTitle: @"Slowest"];
    
	/* Set deinterlaces level according to the integer in the main window */
	[fDeinterlacePopUp selectItemAtIndex: fPictureFilterSettings.deinterlace];

	[fPARCheck setState:(job->pixel_ratio ? NSOnState : NSOffState)];
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
	
	
	
	/* we use a popup to show the denoise settings */
	[fDenoisePopUp removeAllItems];
    [fDenoisePopUp addItemWithTitle: @"None"];
    [fDenoisePopUp addItemWithTitle: @"Weak"];
	[fDenoisePopUp addItemWithTitle: @"Medium"];
    [fDenoisePopUp addItemWithTitle: @"Strong"];
	/* Set denoises level according to the integer in the main window */
	[fDenoisePopUp selectItemAtIndex: fPictureFilterSettings.denoise];
	
    MaxOutputWidth = job->width;
	MaxOutputHeight = job->height;
    fPicture = 0;

    [self SettingsChanged: nil];
}

- (void) Display: (int) anim
{
    hb_get_preview( fHandle, fTitle, fPicture, fBuffer );

    /* Backup previous picture (for effects) */
    memcpy( fTexBuf[1], fTexBuf[0], fTexBufSize );

    if( fHasQE )
    {
        /* Simply copy */
        memcpy( fTexBuf[0], fBuffer, fTexBufSize );
    }
    else
    {
        /* Copy line by line */
        uint8_t * in  = fBuffer;
        uint8_t * out = fTexBuf[0];
		
        for( int i = fTitle->height + 2; i--; )
        {
            memcpy( out, in, 4 * ( fTitle->width + 2 ) );
            in  += 4 * ( fTitle->width + 2 );
            out += 4 * GetAlignedSize( fTitle->width + 2 );
        }
	
    }

    if( [fEffectsCheck state] == NSOffState )
    {
        anim = HB_ANIMATE_NONE;
    }
    else if( [[NSApp currentEvent] modifierFlags] & NSShiftKeyMask )
    {
        anim |= HB_ANIMATE_SLOW;
    }

    [fPictureGLView Display: anim buffer1: fTexBuf[0]
        buffer2: fTexBuf[1] width: ( fTitle->width + 2 )
        height: ( fTitle->height + 2 )];
	
	/* Set the Output Display below the Preview Picture*/
	int titlewidth = fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3];
	int arpwidth = fTitle->job->pixel_aspect_width;
	int arpheight = fTitle->job->pixel_aspect_height;
	int displayparwidth = titlewidth * arpwidth / arpheight;
	int displayparheight = fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1];

    NSSize displaySize = NSMakeSize( (float)fTitle->width, (float)fTitle->height );
    if( fTitle->job->pixel_ratio == 1 )
    {
        [fInfoField setStringValue:[NSString stringWithFormat:
                            @"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d",
                            fTitle->width, fTitle->height, titlewidth,
                            displayparheight, displayparwidth, displayparheight]];
        displaySize.width *= ((float)arpwidth) / ((float)arpheight);
    }
    else
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
    
	if( [fPARCheck state] == NSOnState )
	{
        [fWidthStepper      setIntValue: MaxOutputWidth];
        [fWidthField        setIntValue: MaxOutputWidth];
        
        /* This will show correct anamorphic height values, but
            show distorted preview picture ratio */
        [fHeightStepper      setIntValue: fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1]];
        [fHeightField        setIntValue: fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1]];

        /* if the sender is the Anamorphic checkbox, record the state
           of KeepAspect Ratio so it can be reset if Anamorphic is unchecked again */
        if (sender == fPARCheck)
        {
        keepAspectRatioPreviousState = [fRatioCheck state];
        }
        [fRatioCheck setState:NSOffState];
        [fRatioCheck setEnabled: NO];
        
        [fWidthStepper setEnabled: NO];
        [fWidthField setEnabled: NO];
        [fHeightStepper setEnabled: NO];
        [fHeightField setEnabled: NO];
        
    }
    else
	{
        [fWidthStepper setEnabled: YES];
        [fWidthField setEnabled: YES];
        [fHeightStepper setEnabled: YES];
        [fHeightField setEnabled: YES];
        [fRatioCheck setEnabled: YES];
        /* if the sender is the Anamorphic checkbox, we return the
           keep AR checkbox to its previous state */
        if (sender == fPARCheck)
        {
        [fRatioCheck setState:keepAspectRatioPreviousState];
        }
        
	}
	
    job->width       = [fWidthStepper  intValue];
    job->height      = [fHeightStepper intValue];
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
	job->pixel_ratio = ( [fPARCheck state] == NSOnState );

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
    [fHeightStepper     setIntValue: job->height];
    [fHeightField       setIntValue: job->height];
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
        [self Display: HB_ANIMATE_NONE];
    }
}

- (IBAction) PreviousPicture: (id) sender
{   
    if( fPicture <= 0 )
    {
        return;
    }
    fPicture--;
    [self Display: HB_ANIMATE_BACKWARD];
}

- (IBAction) NextPicture: (id) sender
{
    if( fPicture >= 9 )
    {
        return;
    }
    fPicture++;
    [self Display: HB_ANIMATE_FORWARD];
}

- (IBAction) ClosePanel: (id) sender
{
    if ([delegate respondsToSelector:@selector(pictureSettingsDidChange)])
        [delegate pictureSettingsDidChange];
        
    [NSApp endSheet: fPicturePanel];
    [fPicturePanel orderOut: self];
}

- (BOOL) autoCrop
{
    return autoCrop;
}
- (void) setAutoCrop: (BOOL) setting
{
    autoCrop = setting;
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
    
    [NSApp beginSheet:fPicturePanel
       modalForWindow:fWindow
        modalDelegate:nil
       didEndSelector:nil
          contextInfo:NULL];
}

- (BOOL) loadMyNibFile
{
    if(![NSBundle loadNibNamed:@"PictureSettings" owner:self])
    {
        NSLog(@"Warning! Could not load myNib file.\n");
        return NO;
    }
    
    return YES;
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
    NSSize sheetSize = [fPicturePanel frame].size;
    NSSize viewAreaSize = [fPictureGLViewArea frame].size;
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
    NSSize currentSize = [fPictureGLViewArea frame].size;
    float deltaX = viewSize.width - currentSize.width;
    float deltaY = viewSize.height - currentSize.height;
    
    // Now resize the whole panel by those same deltas, but don't exceed the min
    NSRect frame = [fPicturePanel frame];
    NSSize maxSize = [fPicturePanel maxSize];
    NSSize minSize = [fPicturePanel minSize];
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

    [fPicturePanel setFrame:frame display:YES animate:YES];
}

//
// -[PictureController(Private) setViewSize:]
//
// Changes the OpenGL view's size and centers it vertially inside of its area.
// Assumes resizeSheetForViewSize: has already been called.
//
- (void)setViewSize: (NSSize)viewSize
{
    [fPictureGLView setFrameSize:viewSize];
    
    // center it vertically
    NSPoint origin = [fPictureGLViewArea frame].origin;
    origin.y += ([fPictureGLViewArea frame].size.height -
                 [fPictureGLView frame].size.height) / 2.0;
    [fPictureGLView setFrameOrigin:origin];
}

//
// -[PictureController(Private) viewNeedsToResizeToSize:]
//
// Returns YES if the view will need to resize to match the given size.
//
- (BOOL)viewNeedsToResizeToSize: (NSSize)newSize
{
    NSSize viewSize = [fPictureGLView frame].size;
    return (newSize.width != viewSize.width || newSize.height != viewSize.height);
}


@end
