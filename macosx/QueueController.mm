#include "QueueController.h"

@implementation QueueController

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle = handle;
}

- (void) AddTextField: (NSString *) string rect: (NSRect *) rect
{
    NSTextField * textField;

    rect->origin.x     = 0;
    rect->origin.y    -= 15;
    rect->size.width   = 700;
	rect->size.height  = 15;
    textField = [[NSTextField alloc] initWithFrame: *rect];
    [textField setEditable: NO];
    [textField setSelectable: NO];
    [textField setDrawsBackground: NO];
	[textField setBordered: NO];
	[textField setFont: [NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [textField setStringValue: string];

    [fTaskView addSubview: textField];
	
}

- (void) removeTask: (id) sender
{
    hb_rem( fHandle, hb_job( fHandle, [sender tag] ) );
    [self performSelectorOnMainThread: @selector( Update: )
        withObject: sender waitUntilDone: NO];
}

- (void) AddButton: (NSRect *) rect tag: (int) tag
{
    NSButton * button;
    
    rect->origin.x     = rect->size.width - 60;
    rect->origin.y    -= 20;
	rect->size.width   = 60;
    rect->size.height  = 20;

    button = [[NSButton alloc] initWithFrame: *rect];
    rect->size.width   = rect->origin.x + 90;

    [button setTitle: @"Remove"];
    [button setBezelStyle: NSRoundedBezelStyle];
    [button setFont: [NSFont systemFontOfSize:
        [NSFont systemFontSizeForControlSize: NSMiniControlSize]]];
	[[button cell] setControlSize: NSMiniControlSize];

    [button setTag: tag];
    [button setTarget: self];
    [button setAction: @selector( removeTask: )];

    [fTaskView addSubview: button];

    NSBox * box;

    rect->origin.x     = 15;
    rect->origin.y    -= 10;
    rect->size.width  -= 10;
    rect->size.height  = 1;
    box = [[NSBox alloc] initWithFrame: *rect];
    [box setBoxType: NSBoxSeparator];
    rect->origin.y    -= 10;
   rect->size.width   -= 30;

    [fTaskView addSubview: box];
}

- (IBAction) Update: (id) sender
{
    int i;
    hb_job_t * j;
    hb_title_t * title;

    NSSize size = [fScrollView contentSize];
    int height = MAX( 20 + 145 * hb_count( fHandle ), size.height );
    [fTaskView setFrame: NSMakeRect(0,0,size.width,height)];

    NSRect rect = NSMakeRect(10,height-10,size.width-20,10);

    NSArray * subviews = [fTaskView subviews];
    while( [subviews count] > 0 )
    {
        [[subviews objectAtIndex: 0]
            removeFromSuperviewWithoutNeedingDisplay];
    }

    for( i = 0; i < hb_count( fHandle ); i++ )
    {
        j = hb_job( fHandle, i );
        title = j->title;
       /* show the name of the source Note: use title->name instead of
	   title->dvd since name is just the chosen folder, instead of dvd which is the full path*/ 
        [self AddTextField: [NSString stringWithFormat:
            @"Task: %d Source: %s  Title: %d   Chapters: %d to %d   Pass: %d of %d",i+1, title->name, title->index , j->chapter_start, j->chapter_end,MAX( 1, j->pass ), MIN( 2, j->pass + 1 )] rect: &rect];
       /* Muxer settings (File Format in the gui) */
		if (j->mux == 65536 || j->mux == 131072 || j->mux == 1048576)
		{
		jobFormat = @"MP4"; // HB_MUX_MP4,HB_MUX_PSP,HB_MUX_IPOD
		}
		if (j->mux == 262144)
		{
		jobFormat = @"AVI"; // HB_MUX_AVI
		}
		if (j->mux == 524288)
		{
		jobFormat = @"OGM"; // HB_MUX_OGM
		}
		/* Video Codec settings (Encoder in the gui) */
		if (j->vcodec == 1)
		{
		jobVideoCodec = @"FFmpeg"; // HB_VCODEC_FFMPEG
		}
		if (j->vcodec == 2)
		{
		jobVideoCodec = @"XviD"; // HB_VCODEC_XVID
		}
		if (j->vcodec == 4)
		{
			/* Deterimine for sure how we are now setting iPod uuid atom */
			if (j->h264_level) // We are encoding for iPod
			{
			jobVideoCodec = @"x264 (H.264 iPod)"; // HB_VCODEC_X264	
			}
			else
			{
			jobVideoCodec = @"x264 (H.264 Main)"; // HB_VCODEC_X264
			}
		}
		/* Audio Codecs (Second half of Codecs in the gui) */
		if (j->acodec == 256)
		{
		jobAudioCodec = @"AAC"; // HB_ACODEC_FAAC
		}
		if (j->acodec == 512)
		{
		jobAudioCodec = @"MP3"; // HB_ACODEC_LAME
		}
		if (j->acodec == 1024)
		{
		jobAudioCodec = @"Vorbis"; // HB_ACODEC_VORBIS
		}
		if (j->acodec == 2048)
		{
		jobAudioCodec = @"AC3"; // HB_ACODEC_AC3
		}
		/* Show Basic File info */
		if (j->chapter_markers == 1)
		{
			[self AddTextField: [NSString stringWithFormat: @"Format: %@   Codecs: %@ Video / %@ Audio  Chapter Markers", jobFormat, jobVideoCodec, jobAudioCodec]
						  rect: &rect];
		}
		else
		{
			[self AddTextField: [NSString stringWithFormat: @"Format: %@   Codecs: %@ Video / %@ Audio", jobFormat, jobVideoCodec, jobAudioCodec]
						  rect: &rect];
		}
			
		/*Picture info*/
		/*integers for picture values deinterlace, crop[4], keep_ratio, grayscale, pixel_ratio, pixel_aspect_width, pixel_aspect_height,
		 maxWidth, maxHeight */
		if (j->pixel_ratio == 1)
		{
		int titlewidth = title->width - j->crop[2] - j->crop[3];
		int displayparwidth = titlewidth * j->pixel_aspect_width / j->pixel_aspect_height;
	    int displayparheight = title->height - j->crop[0] - j->crop[1];
		jobPictureDetail = [NSString stringWithFormat: @"Picture: %d x %d Anamorphic", displayparwidth, displayparheight];
		}
		else
		{
		jobPictureDetail = [NSString stringWithFormat: @"Picture: %d x %d", j->width, j->height];
		}
		if (j->keep_ratio == 1)
		{
		jobPictureDetail = [jobPictureDetail stringByAppendingString: @" Keep Aspect Ratio"];
		}
		
		if (j->grayscale == 1)
		{
		jobPictureDetail = [jobPictureDetail stringByAppendingString: @" ,Grayscale"];
		}
		
		if (j->deinterlace == 1)
		{
		jobPictureDetail = [jobPictureDetail stringByAppendingString: @" ,Deinterlace"];
		}
		/* Show Picture info */	
		[self AddTextField: [NSString stringWithFormat: @"%@", jobPictureDetail]rect: &rect];
		
		/* Detailed Video info */
		if (j->vquality <= 0 || j->vquality >= 1)
		{
			jobVideoQuality =[NSString stringWithFormat: @"%d (kbps)", j->vbitrate]; 
		}
		else
		{
		NSNumber * vidQuality;
		vidQuality = [NSNumber numberWithInt: j->vquality * 100];
			/* this is screwed up kind of. Needs to be formatted properly */
			if (j->crf == 1)
			{
				jobVideoQuality =[NSString stringWithFormat: @"%@ %% (crf)", vidQuality];
			}
			else
			{
				jobVideoQuality =[NSString stringWithFormat: @"%@ %% (cqp)", vidQuality];
			}
		}
		
		
		jobVideoDetail = [NSString stringWithFormat:@"Video: %@ %@ FPS: %d", jobVideoCodec, jobVideoQuality, j->vrate / j->vrate_base];
		
		
		/* Add the video detail string to the job filed in the window */
		[self AddTextField: [NSString stringWithFormat:@"%@", jobVideoDetail] rect: &rect];
		
		/* if there is an x264 option string, lets add it here*/
		/*NOTE: Due to size, lets get this in a tool tip*/
		
		if (j->x264opts)
		{
			[self AddTextField: [NSString stringWithFormat:@"x264 Options: %@", [NSString stringWithUTF8String:j->x264opts]] rect: &rect];
		}
		
		/* Audio Detail abitrate arate*/
		if ([jobAudioCodec isEqualToString: @"AC3"])
		{
		jobAudioDetail = [NSString stringWithFormat:@"Audio: %@ Pass-Through", jobAudioCodec];
		}
		else
		{
		jobAudioDetail = [NSString stringWithFormat:@"Audio: %@ Bitrate:%d (kbps) Sample Rate:%d (khz)", jobAudioCodec, j->abitrate, j->arate];
		}
		
		/* we now get the audio mixdown info for each of the two gui audio tracks */
		/* lets do it the long way here to get a handle on things.
			Hardcoded for two tracks for gui: audio_mixdowns[i] audio_mixdowns[i] */
		int ai; // counter for each audios [] , macgui only allows for two audio tracks currently
		for( ai = 0; ai < 2; ai++ )
		{
			if (j->audio_mixdowns[ai] == HB_AMIXDOWN_MONO)
			{ 
				jobAudioDetail = [jobAudioDetail stringByAppendingString: [NSString stringWithFormat:@" ,Track %d: Mono",ai + 1]];
			}
			if (j->audio_mixdowns[ai] == HB_AMIXDOWN_STEREO)
			{ 
				jobAudioDetail = [jobAudioDetail stringByAppendingString: [NSString stringWithFormat:@" ,Track %d: Stero",ai + 1]];
			}
			if (j->audio_mixdowns[ai] == HB_AMIXDOWN_DOLBY)
			{ 
				jobAudioDetail = [jobAudioDetail stringByAppendingString: [NSString stringWithFormat:@" ,Track %d: Dolby Surround",ai + 1]];
			}
			if (j->audio_mixdowns[ai] == HB_AMIXDOWN_DOLBYPLII)
			{ 
				jobAudioDetail = [jobAudioDetail stringByAppendingString: [NSString stringWithFormat:@" ,Track %d: DPL2",ai + 1]];
			}
			if (j->audio_mixdowns[ai] == HB_AMIXDOWN_6CH)
			{ 
				jobAudioDetail = [jobAudioDetail stringByAppendingString: [NSString stringWithFormat:@" ,Track %d: 6 Channel Discreet",ai + 1]];
			}
		}
		//jobAudioDetail = [jobAudioDetail stringByAppendingString: [NSString stringWithFormat:@" ,MixDown: %d",j->audio_mixdowns]];
		/* Add the Audio detail string to the job filed in the window */
		[self AddTextField: [NSString stringWithFormat:@"%@", jobAudioDetail] rect: &rect];
		
		/*Destination Field */
		[self AddTextField: [NSString stringWithFormat: @"Destination: %s", j->file] rect: &rect];
        /* Show remove button */
		[self AddButton: &rect tag: i];
    }

    [fTaskView scrollPoint: NSMakePoint(0,height)];
    [fTaskView setNeedsDisplay: YES];
    
}

- (IBAction) ClosePanel: (id) sender
{
    [NSApp stopModal];
}

@end
