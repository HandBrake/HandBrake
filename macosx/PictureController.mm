/* $Id: PictureController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "PictureController.h"

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
    [fWidthStepper  setMinValue: 16];
    [fHeightStepper setValueWraps: NO];
    [fHeightStepper setIncrement: 16];
    [fHeightStepper setMinValue: 16];

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
    [fDeinterlaceCheck  setState:    job->deinterlace ? NSOnState : NSOffState];
	[fPARCheck  setState:    job->pixel_ratio ? NSOnState : NSOffState];
	if ([fAutoCropMainWindow  intValue] == 0)
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
	if (fTitle->job->pixel_ratio == 1)
	{
	
	[fInfoField setStringValue: [NSString stringWithFormat:
	@"Source: %dx%d, Output: %dx%d, Anamorphic: %dx%d", fTitle->width, fTitle->height,
	titlewidth, displayparheight, displayparwidth,
	displayparheight]];
	
	
	}
	else
	{
	[fInfoField setStringValue: [NSString stringWithFormat:
        @"Source: %dx%d, Output: %dx%d", fTitle->width, fTitle->height,
        fTitle->job->width, fTitle->job->height]];	
	}


    [fPrevButton setEnabled: ( fPicture > 0 )];
    [fNextButton setEnabled: ( fPicture < 9 )];
}

- (IBAction) SettingsChanged: (id) sender
{
    hb_job_t * job = fTitle->job;
    
	if ([fPARCheck state] == 1 )
	{
	[fWidthStepper      setIntValue: MaxOutputWidth];
	[fWidthField        setIntValue: MaxOutputWidth];
	
	/* This will show correct anamorphic height values, but
	show distorted preview picture ratio */
	[fHeightStepper      setIntValue: fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1]];
	[fHeightField        setIntValue: fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1]];
	
	/* This will show wrong anamorphic height values, but
	show proper preview picture ratio */
	//[fHeightStepper      setIntValue: MaxOutputHeight];
	//[fHeightField        setIntValue: MaxOutputHeight];
	[fRatioCheck        setState: 0];

	[fWidthStepper setEnabled: NO];
	[fWidthField setEnabled: NO];
	[fHeightStepper setEnabled: NO];
	[fHeightField setEnabled: NO];
	[fRatioCheck setEnabled: NO];
	
	
	}
	else
	{
	[fWidthStepper setEnabled: YES];
	[fWidthField setEnabled: YES];
	[fHeightStepper setEnabled: YES];
	[fHeightField setEnabled: YES];
	[fRatioCheck setEnabled: YES];
	}
	
	
	
    job->width       = [fWidthStepper  intValue];
    job->height      = [fHeightStepper intValue];
    job->keep_ratio  = ( [fRatioCheck state] == NSOnState );
    job->deinterlace = ( [fDeinterlaceCheck state] == NSOnState );
	job->pixel_ratio = ( [fPARCheck state] == NSOnState );



    bool autocrop = ( [fCropMatrix selectedRow] == 0 );
    [fCropTopStepper    setEnabled: !autocrop];
    [fCropBottomStepper setEnabled: !autocrop];
    [fCropLeftStepper   setEnabled: !autocrop];
    [fCropRightStepper  setEnabled: !autocrop];
	[fAutoCropMainWindow  setStringValue: [NSString stringWithFormat:@"%d",autocrop]];
    if( autocrop )
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
    [self Display: HB_ANIMATE_NONE];
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

	[NSApp stopModal];
}

@end
