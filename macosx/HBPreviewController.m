/* $Id: HBPreviewController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewController.h"
#import "HBPreviewGenerator.h"
#import "HBUtilities.h"
#import "Controller.h"
#import <QTKit/QTKit.h>

#import "HBPicture.h"
#import "HBJob.h"

@implementation QTMovieView (HBQTMovieViewExtensions)

- (void) mouseMoved: (NSEvent *) theEvent
{
    [super mouseMoved:theEvent];
}

@end

@implementation QTMovie (HBQTMovieExtensions)

- (BOOL) isPlaying
{
    if ([self rate])
        return YES;
    else
        return NO;
}

- (NSString *) timecode
{
    QTTime time = [self currentTime];
    double timeInSeconds = (double)time.timeValue / time.timeScale;
	UInt16 seconds = fmod(timeInSeconds, 60.0);
	UInt16 minutes = fmod(timeInSeconds / 60.0, 60.0);
	UInt16 hours = timeInSeconds / (60.0 * 60.0);
	UInt16 milliseconds = (timeInSeconds - (int) timeInSeconds) * 1000;
	return [NSString stringWithFormat:@"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds];
}

- (void) setCurrentTimeDouble: (double) value
{
	long timeScale = [[self attributeForKey:QTMovieTimeScaleAttribute] longValue];
	[self setCurrentTime:QTMakeTime(value * timeScale, timeScale)];
}

@end

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
@interface NSWindow(HBExtensions)
@property (readonly) CGFloat backingScaleFactor;
@end
#endif

#define BORDER_SIZE 2.0
// make min width and height of preview window large enough for hud
#define MIN_WIDTH 480.0
#define MIN_HEIGHT 360.0

#define ANIMATION_DUR 0.15

typedef enum ViewMode : NSUInteger {
    ViewModePicturePreview,
    ViewModeEncoding,
    ViewModeMoviePreview
} ViewMode;

@interface HBPreviewController () <HBPreviewGeneratorDelegate>
{
    /* HUD boxes */
    IBOutlet NSView           * fPictureControlBox;
    IBOutlet NSView           * fEncodingControlBox;
    IBOutlet NSView           * fMoviePlaybackControlBox;

    IBOutlet NSSlider        * fPictureSlider;
    IBOutlet NSTextField     * fInfoField;
    IBOutlet NSTextField     * fscaleInfoField;

    /* Full Screen Mode Toggle */
    IBOutlet NSButton               * fScaleToScreenToggleButton;

    /* Movie Previews */
    IBOutlet QTMovieView            * fMovieView;
    /* Playback Panel Controls */
    IBOutlet NSButton               * fPlayPauseButton;
    IBOutlet NSSlider               * fMovieScrubberSlider;
    IBOutlet NSTextField            * fMovieInfoField;

    IBOutlet NSProgressIndicator    * fMovieCreationProgressIndicator;
    IBOutlet NSTextField            * fPreviewMovieStatusField;

    /* Popup of choices for length of preview in seconds */
    IBOutlet NSPopUpButton          * fPreviewMovieLengthPopUp;
}

@property (nonatomic) CALayer *backLayer;
@property (nonatomic) CALayer *pictureLayer;

@property (nonatomic) CGFloat backingScaleFactor;

@property (nonatomic) ViewMode currentViewMode;
@property (nonatomic) BOOL scaleToScreen;

@property (nonatomic, retain) NSTimer *hudTimer;

@property (nonatomic, retain) HBPreviewGenerator *generator;
@property (nonatomic) NSUInteger pictureIndex;

@property (nonatomic, retain) QTMovie *movie;
@property (nonatomic, retain) NSTimer *movieTimer;

/* Pictures HUD actions */
- (IBAction) previewDurationPopUpChanged: (id) sender;
- (IBAction) pictureSliderChanged: (id) sender;
- (IBAction) showPictureSettings:(id)sender;
- (IBAction) toggleScaleToScreen:(id)sender;

- (IBAction) cancelCreateMoviePreview: (id) sender;
- (IBAction) createMoviePreview: (id) sender;

/* Movie HUD actions */
- (IBAction) showPicturesPreview: (id) sender;
- (IBAction) toggleMoviePreviewPlayPause: (id) sender;
- (IBAction) moviePlaybackGoToBeginning: (id) sender;
- (IBAction) moviePlaybackGoToEnd: (id) sender;
- (IBAction) previewScrubberChanged: (id) sender;

@end

@implementation HBPreviewController

- (id) init
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

    }
	return self;
}

- (void) awakeFromNib
{
    [[self window] setDelegate:self];

    if( ![[self window] setFrameUsingName:@"Preview"] )
        [[self window] center];

    [self setWindowFrameAutosaveName:@"Preview"];
    [[self window] setExcludedFromWindowsMenu:YES];

    /* lets set the preview window to accept mouse moved events */
    [[self window] setAcceptsMouseMovedEvents:YES];

    /* we set the progress indicator to not use threaded animation
     * as it causes a conflict with the qtmovieview's controllerbar
     */
    [fMovieCreationProgressIndicator setUsesThreadedAnimation:NO];

	[fMovieView setHidden:YES];
    [fMovieView setDelegate:self];
    [fMovieView setControllerVisible:NO];

    /* we set the preview length popup in seconds */
    [fPreviewMovieLengthPopUp removeAllItems];
    [fPreviewMovieLengthPopUp addItemsWithTitles:@[@"15", @"30", @"45", @"60", @"90",
                                                   @"120", @"150", @"180", @"210", @"240"]];

    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewLength"])
        [fPreviewMovieLengthPopUp selectItemWithTitle:[[NSUserDefaults standardUserDefaults]
                                                       objectForKey:@"PreviewLength"]];
    if (![fPreviewMovieLengthPopUp selectedItem])
        /* currently hard set default to 15 seconds */
        [fPreviewMovieLengthPopUp selectItemAtIndex: 0];

    /* Setup our layers for core animation */
    [[[self window] contentView] setWantsLayer:YES];

    self.backLayer = [CALayer layer];
    [self.backLayer setBounds:CGRectMake(0.0, 0.0, MIN_WIDTH, MIN_HEIGHT)];
    [self.backLayer setPosition:CGPointMake([[[self window] contentView] frame].size.width /2,
                                            [[[self window] contentView] frame].size.height /2)];

    [self.backLayer setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
    CGColorRef white = CGColorCreateGenericRGB(1.0, 1.0, 1.0, 1.0);
    [self.backLayer setBackgroundColor: white];
    CFRelease(white);
    [self.backLayer setShadowOpacity:0.5f];
    [self.backLayer setShadowOffset:CGSizeMake(0, 0)];

    self.pictureLayer = [CALayer layer];
    [self.pictureLayer setBounds:CGRectMake(0.0, 0.0, MIN_WIDTH - (BORDER_SIZE * 2), MIN_HEIGHT - (BORDER_SIZE * 2))];
    [self.pictureLayer setPosition:CGPointMake([[[self window] contentView] frame].size.width /2,
                                               [[[self window] contentView] frame].size.height /2)];

    [self.pictureLayer setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];

    // Disable fade on contents change
    NSMutableDictionary *actions = [NSMutableDictionary
                                    dictionaryWithDictionary:[self.pictureLayer actions]];

    [actions setObject:[NSNull null] forKey:@"contents"];
    [self.pictureLayer setActions:actions];

    [[[[self window] contentView] layer] insertSublayer:self.backLayer below: [fMovieView layer]];
    [[[[self window] contentView] layer] insertSublayer:self.pictureLayer below: [fMovieView layer]];

    /* relocate our hud origins */
    NSPoint hudControlBoxOrigin = [fMoviePlaybackControlBox frame].origin;
    [fPictureControlBox setFrameOrigin:hudControlBoxOrigin];
    [fEncodingControlBox setFrameOrigin:hudControlBoxOrigin];
    [fMoviePlaybackControlBox setFrameOrigin:hudControlBoxOrigin];

    /* set the current scale factor */
    if( [[self window] respondsToSelector:@selector( backingScaleFactor )] )
        self.backingScaleFactor = [[self window] backingScaleFactor];
    else
        self.backingScaleFactor = 1.0;
}

- (void) setJob:(HBJob *)job
{
    _job = job;

    self.generator.delegate = nil;
    [self.generator cancel];
    self.generator = nil;

    if (job)
    {
        /* alloc and init a generator for the current title */
        self.generator = [[[HBPreviewGenerator alloc] initWithCore:self.core job:self.job] autorelease];

        /* adjust the preview slider length */
        [fPictureSlider setMaxValue: self.generator.imagesCount - 1.0];
        [fPictureSlider setNumberOfTickMarks: self.generator.imagesCount];

        [self switchViewToMode:ViewModePicturePreview];
        [self displayPreview];
    }
}

- (void) reloadPreviews
{
    if (self.job)
    {
        // Purge the existing picture previews so they get recreated the next time
        // they are needed.
        [self.generator purgeImageCache];
        [self switchViewToMode:ViewModePicturePreview];
        [self displayPreview];
    }
}

- (void) showWindow: (id) sender
{
    [super showWindow:sender];

    if (self.currentViewMode == ViewModeMoviePreview)
    {
        [self startMovieTimer];
    }
    else
    {
        [self reloadPreviews];
    }
}

- (void) windowWillClose: (NSNotification *) aNotification
{
    if (self.currentViewMode == ViewModeEncoding)
    {
        [self cancelCreateMoviePreview:self];
    }
    else if (self.currentViewMode == ViewModeMoviePreview)
    {
        [fMovieView pause:self];
        [self stopMovieTimer];
    }

    [self.generator purgeImageCache];
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PreviewWindowIsOpen"];
}

- (void) windowDidChangeBackingProperties: (NSNotification *) notification
{
    NSWindow *theWindow = (NSWindow *)[notification object];

    CGFloat newBackingScaleFactor = [theWindow backingScaleFactor];
    CGFloat oldBackingScaleFactor = [[[notification userInfo]
                                      objectForKey:@"NSBackingPropertyOldScaleFactorKey"]
                                     doubleValue];

    if (newBackingScaleFactor != oldBackingScaleFactor)
    {
        // Scale factor changed, update the preview window
        // to the new situation
        self.backingScaleFactor = newBackingScaleFactor;
        if (self.job)
            [self reloadPreviews];
    }
}

/**
 * Given the size of the preview image to be shown, returns the best possible
 * size for the view.
 */
- (NSSize) optimalViewSizeForImageSize: (NSSize) imageSize
{
    CGFloat minWidth = MIN_WIDTH;
    CGFloat minHeight = MIN_HEIGHT;

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

/**
 * Resizes the entire window to accomodate a view of a particular size.
 */
- (void) resizeWindowForViewSize: (NSSize) viewSize
{
    NSSize currentSize = [[[self window] contentView] frame].size;
    NSRect frame = [[self window] frame];

    // Calculate border around content region of the frame
    int borderX = frame.size.width - currentSize.width;
    int borderY = frame.size.height - currentSize.height;

    // Make sure the frame is smaller than the screen
    NSSize maxSize = [[[self window] screen] visibleFrame].size;

    /* if we are not Scale To Screen, put an 10% of visible screen on the window */
    if (self.scaleToScreen == NO)
    {
        maxSize.width = maxSize.width * 0.90;
        maxSize.height = maxSize.height * 0.90;
    }

    // Set the new frame size
    // Add the border to the new frame size so that the content region
    // of the frame is large enough to accomodate the preview image
    frame.size.width = viewSize.width + borderX;
    frame.size.height = viewSize.height + borderY;

    /* compare frame to max size of screen */
    if( frame.size.width > maxSize.width )
    {
        frame.size.width = maxSize.width;
    }
    if( frame.size.height > maxSize.height )
    {
        frame.size.height = maxSize.height;
    }

    /* Since upon launch we can open up the preview window if it was open
     * the last time we quit (and at the size it was) we want to make
     * sure that upon resize we do not have the window off the screen
     * So check the origin against the screen origin and adjust if
     * necessary.
     */
    NSSize screenSize = [[[self window] screen] visibleFrame].size;
    NSPoint screenOrigin = [[[self window] screen] visibleFrame].origin;

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

    if (self.scaleToScreen == YES)
    {
        /* our origin is off the screen to the top*/
        if (frame.origin.y < screenOrigin.y)
        {
            /* so shift our origin to the bottom */
            frame.origin.y = screenOrigin.y;
        }
        else if ((frame.origin.y + frame.size.height) > (screenOrigin.y + screenSize.height))
        {
            /* the top side of the preview is off the screen, so shift to the bottom */
            frame.origin.y = (screenOrigin.y + screenSize.height) - frame.size.height;
        }
    }

    [[self window] setFrame:frame display:YES animate:YES];
}


/**
 * Enable/Disable an arbitrary number of UI elements.
 * @param boxes an array of UI elements
 * @param indexes a set of indexes of the elements in boxes to be enabled
 */
- (void) toggleBoxes: (NSArray *) boxes usingIndexes: (NSIndexSet *) indexes
{
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:ANIMATION_DUR];

    [boxes enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        BOOL hide = [indexes containsIndex:idx] ? NO : YES;
        if (hide)
        {
            [self hideHudWithAnimation:obj];
        }
        else
        {
            [self showHudWithAnimation:obj];
        }
    }];

    [NSAnimationContext endGrouping];
}

/**
 * Switch the preview controller to one of his view mode:
 * ViewModePicturePreview, ViewModeEncoding, ViewModeMoviePreview
 * This methods is the only way to change the mode, do not try otherwise.
 * @param mode ViewMode mode
 */
- (void) switchViewToMode: (ViewMode) mode
{
    switch (mode) {
        case ViewModePicturePreview:
        {
            if (self.currentViewMode == ViewModeEncoding)
            {
                [self toggleBoxes:@[fPictureControlBox, fEncodingControlBox]
                     usingIndexes:[NSIndexSet indexSetWithIndex:0]];
                [fMovieCreationProgressIndicator stopAnimation:self];
            }
            else if (self.currentViewMode == ViewModeMoviePreview)
            {
                /* Stop playback and remove the observers */
                [fMovieView pause:self];
                [self stopMovieTimer];
                [self removeMovieObservers];

                [self toggleBoxes:@[fPictureControlBox, fMoviePlaybackControlBox, fMovieView]
                     usingIndexes:[NSIndexSet indexSetWithIndex:0]];

                /* Release the movie */
                [fMovieView setMovie:nil];
                self.movie = nil;
            }

            break;
        }

        case ViewModeEncoding:
        {
            [fMovieCreationProgressIndicator setDoubleValue:0];
            [fMovieCreationProgressIndicator startAnimation:self];
            [self toggleBoxes:@[fEncodingControlBox, fPictureControlBox, fMoviePlaybackControlBox]
                 usingIndexes:[NSIndexSet indexSetWithIndex:0]];

            break;
        }

        case ViewModeMoviePreview:
        {
            [self toggleBoxes:@[fMovieView, fMoviePlaybackControlBox, fEncodingControlBox, fPictureControlBox]
                 usingIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, 2)]];

            [fMovieCreationProgressIndicator stopAnimation:self];
            [self initPreviewScrubberForMovie];
            [self startMovieTimer];

            /* Install movie notifications */
            [self addMovieObservers];
        }
            break;

        default:
            break;
    }

    self.currentViewMode = mode;
}

- (void) dealloc
{
    [_hudTimer invalidate];
    [_hudTimer release];

    [_movieTimer invalidate];
    [_movieTimer release];

    [_generator cancel];
    [_generator release];

    [self removeMovieObservers];

    [super dealloc];
}

#pragma mark -
#pragma mark Hud Control Overlay

- (void) mouseMoved: (NSEvent *) theEvent
{
    [super mouseMoved:theEvent];
    NSPoint mouseLoc = [theEvent locationInWindow];

    /* Test for mouse location to show/hide hud controls */
    if (self.currentViewMode != ViewModeEncoding && self.job)
    {
        /* Since we are not encoding, verify which control hud to show
         * or hide based on aMovie ( aMovie indicates we need movie controls )
         */
        NSView *hud;
        if (self.currentViewMode == !ViewModeMoviePreview) // No movie loaded up
        {
            hud = fPictureControlBox;
        }
        else // We have a movie
        {
            hud = fMoviePlaybackControlBox;
        }

        if (NSPointInRect(mouseLoc, [hud frame]))
        {
            [self showHudWithAnimation:hud];
            [self stopHudTimer];
        }
		else if (NSPointInRect(mouseLoc, [[[self window] contentView] frame]))
        {
            [self showHudWithAnimation:hud];
            [self startHudTimer];
        }
        else
        {
            [self hideHudWithAnimation:hud];
            [self stopHudTimer];
        }
	}
}

- (void)showHudWithAnimation:(NSView *)hud
{
    // The standard view animator doesn't play
    // nicely with the Yosemite visual effects yet.
    // So let's do the fade ourself.
    if (hud.layer.opacity == 0 || [hud isHidden])
    {
        [hud setHidden:NO];

        [CATransaction begin];
        CABasicAnimation *fadeInAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        fadeInAnimation.fromValue = @(0.0);
        fadeInAnimation.toValue = @(1.0);
        fadeInAnimation.beginTime = 0.0;
        fadeInAnimation.duration = ANIMATION_DUR;

        [hud.layer addAnimation:fadeInAnimation forKey:nil];
        [hud.layer setOpacity:1];

        [CATransaction commit];
    }
}

- (void)hideHudWithAnimation:(NSView *)hud
{
    if (hud.layer.opacity != 0)
    {
        [CATransaction begin];
        [CATransaction setCompletionBlock:^{
            if (hud.layer.opacity == 0)
            {
                [hud setHidden:YES];
            }
        }];
        CABasicAnimation *fadeInAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        fadeInAnimation.fromValue = @([hud.layer.presentationLayer opacity]);
        fadeInAnimation.toValue = @(0.0);
        fadeInAnimation.beginTime = 0.0;
        fadeInAnimation.duration = ANIMATION_DUR;

        [hud.layer addAnimation:fadeInAnimation forKey:nil];
        [hud.layer setOpacity:0];

        [CATransaction commit];
    }
}

- (void) startHudTimer
{
	if (self.hudTimer)
    {
        [self.hudTimer setFireDate:[NSDate dateWithTimeIntervalSinceNow:8.0]];
	}
    else
    {
        self.hudTimer = [NSTimer scheduledTimerWithTimeInterval:8.0 target:self selector:@selector(hudTimerFired:)
                                                       userInfo:nil repeats:YES];
    }
}

- (void) stopHudTimer
{
    [self.hudTimer invalidate];
    self.hudTimer = nil;
}

- (void) hudTimerFired: (NSTimer *)theTimer
{
    /* Regardless which control box is active, after the timer
     * period we want either one to fade to hidden.
     */
    [self hideHudWithAnimation:fPictureControlBox];
    [self hideHudWithAnimation:fMoviePlaybackControlBox];

    [self stopHudTimer];
}

#pragma mark -
#pragma mark Still previews mode

/**
 * Adjusts the window to draw the current picture (fPicture) adjusting its size as
 * necessary to display as much of the picture as possible.
 */
- (void) displayPreview
{
    if (self.window.isVisible)
    {
        NSImage *fPreviewImage = [self.generator imageAtIndex:self.pictureIndex shouldCache:YES];
        [self.pictureLayer setContents:fPreviewImage];
    }

    HBPicture *pict = self.job.picture;

    /* Set the picture size display fields below the Preview Picture*/
    int display_width = pict.width * pict.parWidth / pict.parHeight;
    NSSize imageScaledSize = NSMakeSize(display_width, pict.height);
    NSSize displaySize = NSMakeSize(display_width, pict.height);

    if (self.backingScaleFactor != 1.0)
    {
        // HiDPI mode usually display everything
        // with douple pixel count, but we don't
        // want to double the size of the video
        displaySize.height /= self.backingScaleFactor;
        displaySize.width /= self.backingScaleFactor;
        imageScaledSize.height /= self.backingScaleFactor;
        imageScaledSize.width /= self.backingScaleFactor;
    }

    // Get the optimal view size for the image
    NSSize viewSize = [self optimalViewSizeForImageSize:displaySize];
    viewSize.width += BORDER_SIZE * 2;
    viewSize.height += BORDER_SIZE * 2;

    NSSize windowSize;
    if (self.scaleToScreen == YES)
        // Scale the window to the max possible size
        windowSize = [[[self window] screen] visibleFrame].size;
    else
        // Scale the window to the image size
        windowSize = viewSize;

    [self resizeWindowForViewSize:windowSize];
    NSSize areaSize = [[[self window] contentView] frame].size;
    areaSize.width -= BORDER_SIZE * 2;
    areaSize.height -= BORDER_SIZE * 2;

    if (self.scaleToScreen == YES)
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
    [self.backLayer setBounds:CGRectMake(0, 0, viewSize.width + (BORDER_SIZE * 2), viewSize.height + (BORDER_SIZE * 2))];
    [self.pictureLayer setBounds:CGRectMake(0, 0, viewSize.width, viewSize.height)];

    NSString *scaleString;
    CGFloat scale = ( ( CGFloat )[self.pictureLayer frame].size.width) / ( ( CGFloat )imageScaledSize.width);
    if (scale * 100.0 != 100)
        scaleString = [NSString stringWithFormat:@" (%.0f%% actual size)", scale * 100.0];
    else
        scaleString = @"(Actual size)";

    if (_scaleToScreen == YES)
        scaleString = [scaleString stringByAppendingString:@" Scaled To Screen"];

    /* Set the info fields in the hud controller */
    [fInfoField setStringValue: [NSString stringWithFormat:
                                 @"%@", self.job.picture.info]];

    [fscaleInfoField setStringValue: [NSString stringWithFormat:
                                      @"%@", scaleString]];

    /* Set the info field in the window title bar */
    [[self window] setTitle:[NSString stringWithFormat: @"Preview - %@ %@", self.job.picture.info, scaleString]];
}

- (IBAction) previewDurationPopUpChanged: (id) sender
{
    [[NSUserDefaults standardUserDefaults] setObject:[fPreviewMovieLengthPopUp titleOfSelectedItem] forKey:@"PreviewLength"];
}

- (IBAction) pictureSliderChanged: (id) sender
{
    if ((self.pictureIndex != [fPictureSlider intValue] || !sender) && self.job) {
        self.pictureIndex = [fPictureSlider intValue];
        [self displayPreview];
    }
}

- (IBAction) toggleScaleToScreen: (id) sender
{
    if (self.scaleToScreen == YES)
    {
        self.scaleToScreen = NO;
        /* make sure we are set to a still preview */
        [self displayPreview];
        [fScaleToScreenToggleButton setTitle:@"Scale To Screen"];
    }
    else
    {
        self.scaleToScreen = YES;
        /* make sure we are set to a still preview */
        [self displayPreview];
        [fScaleToScreenToggleButton setTitle:@"Actual Scale"];
    }
}

- (IBAction) showPictureSettings: (id) sender
{
    [self.delegate showPicturePanel:self];
}

#pragma mark -
#pragma mark Movie preview mode

- (void) updateProgress: (double) progress info: (NSString *) progressInfo {
    [fPreviewMovieStatusField setStringValue: progressInfo];

    [fMovieCreationProgressIndicator setIndeterminate: NO];
    [fMovieCreationProgressIndicator setDoubleValue: progress];
}

- (void)didCancelMovieCreation
{
    [self switchViewToMode:ViewModePicturePreview];
}

- (void) didCreateMovieAtURL: (NSURL *) fileURL
{
    /* Load the new movie into fMovieView */
    if (fileURL)
    {
		NSError *outError;
		NSDictionary *movieAttributes = @{QTMovieURLAttribute: fileURL,
                                          QTMovieAskUnresolvedDataRefsAttribute: @(NO),
                                          @"QTMovieOpenForPlaybackAttribute": @(YES),
                                          @"QTMovieOpenAsyncRequiredAttribute": @(NO),
                                          @"QTMovieOpenAsyncOKAttribute": @(NO),
                                          @"QTMovieIsSteppableAttribute": @(YES),
                                          QTMovieApertureModeAttribute: QTMovieApertureModeClean};

        QTMovie *movie = [[[QTMovie alloc] initWithAttributes:movieAttributes error:&outError] autorelease];

		if (!movie)
        {
            [HBUtilities writeToActivityLog: "showMoviePreview: Unable to open movie"];
            [self switchViewToMode:ViewModePicturePreview];
		}
        else
        {
            /* Scale the fMovieView to the picture player size */
            [fMovieView setFrameSize:[self.pictureLayer frame].size];
            [fMovieView setFrameOrigin:[self.pictureLayer frame].origin];

            [fMovieView setMovie:movie];
            [movie setDelegate:self];

            // get and enable subtitles
            NSArray *subtitlesArray = [movie tracksOfMediaType: @"sbtl"];
            if (subtitlesArray && [subtitlesArray count])
            {
                // enable the first tx3g subtitle track
                [[subtitlesArray objectAtIndex: 0] setEnabled: YES];
            }
            else
            {
                // Perian subtitles
                subtitlesArray = [movie tracksOfMediaType: QTMediaTypeVideo];
                if (subtitlesArray && ([subtitlesArray count] >= 2))
                {
                    // track 0 should be video, other video tracks should
                    // be subtitles; force-enable the first subs track
                    [[subtitlesArray objectAtIndex: 1] setEnabled: YES];
                }
            }

            // to actually play the movie
            self.movie = movie;

            [self switchViewToMode:ViewModeMoviePreview];

            [fMovieView play:movie];
        }
    }
}

- (IBAction) cancelCreateMoviePreview: (id) sender
{
    [self.generator cancel];
}

- (IBAction) createMoviePreview: (id) sender
{
    if (!self.generator)
        return;

    self.generator.delegate = self;
    [self.delegate prepareJobForPreview];
    [self.generator createMovieAsyncWithImageIndex:self.pictureIndex
                                       andDuration:[[fPreviewMovieLengthPopUp titleOfSelectedItem] intValue]];

    [self switchViewToMode:ViewModeEncoding];
}

- (IBAction) toggleMoviePreviewPlayPause: (id) sender
{
    /* make sure a movie is even loaded up */
    if (self.movie)
    {
        if ([self.movie isPlaying]) // we are playing
        {
            [fMovieView pause:self.movie];
            [fPlayPauseButton setState: NSOnState];
        }
        else // we are paused or stopped
        {
            [fMovieView play:self.movie];
            [fPlayPauseButton setState: NSOffState];
        }
    }
}

- (IBAction) moviePlaybackGoToBeginning: (id) sender
{
    [fMovieView gotoBeginning:self.movie];
}

- (IBAction) moviePlaybackGoToEnd: (id) sender
{
    [fMovieView gotoEnd:self.movie];
}

- (void) startMovieTimer
{
	if (!self.movieTimer)
    {
        self.movieTimer = [NSTimer scheduledTimerWithTimeInterval:0.09 target:self
                                                         selector:@selector(movieTimerFired:)
                                                         userInfo:nil repeats:YES];
    }
}

- (void) stopMovieTimer
{
    [self.movieTimer invalidate];
    self.movieTimer = nil;
}

- (void) movieTimerFired: (NSTimer *)theTimer
{
    if (self.movie != nil)
    {
        [self adjustPreviewScrubberForCurrentMovieTime];
        [fMovieInfoField setStringValue: [self.movie timecode]];
    }
}

- (IBAction) showPicturesPreview: (id) sender
{
    [self switchViewToMode:ViewModePicturePreview];
}

#pragma mark -
#pragma mark Movie Playback Scrubber

// Initialize the preview scrubber min/max to appropriate values for the current movie
- (void) initPreviewScrubberForMovie
{
    QTTime duration = [self.movie duration];
    CGFloat result = duration.timeValue / duration.timeScale;

    [fMovieScrubberSlider setMinValue:0.0];
    [fMovieScrubberSlider setMaxValue: result];
    [fMovieScrubberSlider setDoubleValue: 0.0];
}

- (void) adjustPreviewScrubberForCurrentMovieTime
{
    QTTime time = [self.movie currentTime];

    CGFloat result = (CGFloat)time.timeValue / (CGFloat)time.timeScale;;
    [fMovieScrubberSlider setDoubleValue:result];
}

- (IBAction) previewScrubberChanged: (id) sender
{
    [fMovieView pause:self.movie];
    [self.movie setCurrentTimeDouble:[fMovieScrubberSlider doubleValue]];
    [fMovieInfoField setStringValue: [self.movie timecode]];
}

#pragma mark -
#pragma mark Movie Notifications

- (void) addMovieObservers
{
    /* Notification for any time the movie rate changes */
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(movieRateDidChange:)
                                                 name:@"QTMovieRateDidChangeNotification"
                                               object:self.movie];
}

- (void) removeMovieObservers
{
    /*Notification for any time the movie rate changes */
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:@"QTMovieRateDidChangeNotification"
                                                  object:self.movie];
}

- (void) movieRateDidChange: (NSNotification *) notification
{
    if ([self.movie isPlaying])
        [fPlayPauseButton setState: NSOnState];
    else
        [fPlayPauseButton setState: NSOffState];
}

#pragma mark -
#pragma mark Keyboard and mouse wheel control

/* fMovieView Keyboard controls */
- (void) keyDown: (NSEvent *) event
{
    unichar key = [[event charactersIgnoringModifiers] characterAtIndex:0];
    QTMovie *movie = self.movie;

    if (movie)
    {
        if (key == 32)
        {
            if ([movie isPlaying])
                [fMovieView pause:movie];
            else
                [fMovieView play:movie];
        }
        else if (key == 'k')
            [fMovieView pause:movie];
        else if (key == 'l')
        {
            float rate = [movie rate];
            rate += 1.0f;
            [fMovieView play:movie];
            [movie setRate:rate];
        }
        else if (key == 'j')
        {
            float rate = [movie rate];
            rate -= 1.0f;
            [fMovieView play:movie];
            [movie setRate:rate];
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
    else if (self.currentViewMode != ViewModeEncoding)
    {
        if (key == NSLeftArrowFunctionKey)
        {
            [fPictureSlider setIntegerValue:self.pictureIndex > [fPictureSlider minValue] ? self.pictureIndex - 1 : self.pictureIndex];
            [self pictureSliderChanged:self];
        }
        else if (key == NSRightArrowFunctionKey)
        {
            [fPictureSlider setIntegerValue:self.pictureIndex < [fPictureSlider maxValue] ? self.pictureIndex + 1 : self.pictureIndex];
            [self pictureSliderChanged:self];
        }
        else
            [super keyDown:event];
    }
    else
        [super keyDown:event];
}

- (void) scrollWheel: (NSEvent *) theEvent
{
    if (self.currentViewMode != ViewModeEncoding)
    {
        if ([theEvent deltaY] < 0)
        {
            [fPictureSlider setIntegerValue:self.pictureIndex < [fPictureSlider maxValue] ? self.pictureIndex + 1 : self.pictureIndex];
            [self pictureSliderChanged:self];
        }
        else if ([theEvent deltaY] > 0)
        {
            [fPictureSlider setIntegerValue:self.pictureIndex > [fPictureSlider minValue] ? self.pictureIndex - 1 : self.pictureIndex];
            [self pictureSliderChanged:self];
        }
    }
}

@end
