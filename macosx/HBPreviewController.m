/* $Id: HBPreviewController.mm,v 1.11 2005/08/01 15:10:44 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewController.h"
#import "HBPreviewGenerator.h"
#import "HBPictureController.h"

#import "HBPreviewView.h"

#import <QTKit/QTKit.h>
#import "QTKit+HBQTMovieExtensions.h"

#define ANIMATION_DUR 0.15

// make min width and height of preview window large enough for hud
#define MIN_WIDTH 480.0
#define MIN_HEIGHT 360.0

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

@property (nonatomic, readwrite) HBPictureController *pictureSettingsWindow;

@property (nonatomic) ViewMode currentViewMode;
@property (nonatomic) NSPoint windowCenterPoint;

@property (nonatomic, strong) NSTimer *hudTimer;

@property (nonatomic) NSUInteger pictureIndex;

@property (nonatomic, strong) QTMovie *movie;
@property (nonatomic, strong) NSTimer *movieTimer;

@property (weak) IBOutlet HBPreviewView *previewView;

@end

@implementation HBPreviewController

- (instancetype)init
{
    self = [super initWithWindowNibName:@"PicturePreview"];
	return self;
}

- (void)windowDidLoad
{
    [self.window.contentView setWantsLayer:YES];

    // Read the window center position
    // We need the center and we can't use the
    // standard NSWindow autosave because we change
    // the window size at startup.
    NSString *centerString = [[NSUserDefaults standardUserDefaults] objectForKey:@"HBPreviewWindowCenter"];
    if (centerString.length)
    {
        NSPoint center = NSPointFromString(centerString);
        NSRect frame = self.window.frame;
        [self.window setFrameOrigin:NSMakePoint(center.x - floor(frame.size.width / 2),
                                                center.y - floor(frame.size.height / 2))];

        self.windowCenterPoint = center;
    }
    else
    {
        self.windowCenterPoint = [self centerPoint];
    }

    self.window.excludedFromWindowsMenu = YES;
    self.window.acceptsMouseMovedEvents = YES;

    // we set the preview length popup in seconds
    [fPreviewMovieLengthPopUp removeAllItems];
    [fPreviewMovieLengthPopUp addItemsWithTitles:@[@"15", @"30", @"45", @"60", @"90",
                                                   @"120", @"150", @"180", @"210", @"240"]];

    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewLength"])
    {
        [fPreviewMovieLengthPopUp selectItemWithTitle:[[NSUserDefaults standardUserDefaults]
                                                       objectForKey:@"PreviewLength"]];
    }
    if (!fPreviewMovieLengthPopUp.selectedItem)
    {
        // currently hard set default to 15 seconds
        [fPreviewMovieLengthPopUp selectItemAtIndex: 0];
    }

    // Relocate our hud origins.
    NSPoint hudControlBoxOrigin = fMoviePlaybackControlBox.frame.origin;
    fPictureControlBox.frameOrigin = hudControlBoxOrigin;
    fEncodingControlBox.frameOrigin = hudControlBoxOrigin;
    fMoviePlaybackControlBox.frameOrigin = hudControlBoxOrigin;

    [self hideHud];

    fMovieView.hidden = YES;
    fMovieView.delegate = self;
    [fMovieView setControllerVisible:NO];
}

- (void)dealloc
{
    [self removeMovieObservers];

    [_hudTimer invalidate];
    [_movieTimer invalidate];
    [_generator cancel];
}

- (void)setPicture:(HBPicture *)picture
{
    _picture = picture;
    self.pictureSettingsWindow.picture = _picture;
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    if (_generator)
    {
        _generator.delegate = nil;
        [_generator cancel];
    }

    _generator = generator;

    if (generator)
    {
        generator.delegate = self;

        // adjust the preview slider length
        [fPictureSlider setMaxValue: generator.imagesCount - 1.0];
        [fPictureSlider setNumberOfTickMarks: generator.imagesCount];

        if (self.pictureIndex > generator.imagesCount)
        {
            self.pictureIndex = generator.imagesCount - 1;
        }

        [self switchViewToMode:ViewModePicturePreview];
        [self displayPreviewAtIndex:self.pictureIndex];
    }
    else
    {
        self.previewView.image = nil;
        self.window.title = NSLocalizedString(@"Preview", nil);
    }
}

- (void)reloadPreviews
{
    if (self.generator)
    {
        [self switchViewToMode:ViewModePicturePreview];
        [self displayPreviewAtIndex:self.pictureIndex];
    }
}

- (void)showWindow:(id)sender
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

- (void)windowWillClose:(NSNotification *)aNotification
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

    [self.pictureSettingsWindow close];
    [self.generator purgeImageCache];
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
    NSWindow *theWindow = (NSWindow *)[notification object];

    CGFloat newBackingScaleFactor = [theWindow backingScaleFactor];
    CGFloat oldBackingScaleFactor = [[notification userInfo][@"NSBackingPropertyOldScaleFactorKey"]
                                     doubleValue];

    if (newBackingScaleFactor != oldBackingScaleFactor)
    {
        // Scale factor changed, update the preview window
        // to the new situation
        if (self.generator)
        {
            [self reloadPreviews];
        }
    }
}

#pragma mark - Window sizing

/**
 *  Calculates and returns the center point of the window
 */
- (NSPoint)centerPoint {
    NSPoint center = NSMakePoint(floor(self.window.frame.origin.x + self.window.frame.size.width / 2),
                                 floor(self.window.frame.origin.y + self.window.frame.size.height / 2));
    return center;
}

- (void)windowDidMove:(NSNotification *)notification
{
    if (self.previewView.fitToView == NO)
    {
        self.windowCenterPoint = [self centerPoint];
        [[NSUserDefaults standardUserDefaults] setObject:NSStringFromPoint(self.windowCenterPoint) forKey:@"HBPreviewWindowCenter"];
    }
}

- (void)windowDidResize:(NSNotification *)notification
{
    [self updateSizeLabels];
}

/**
 * Resizes the entire window to accomodate a view of a particular size.
 */
- (void)resizeWindowForViewSize:(NSSize)viewSize animate:(BOOL)performAnimation
{
    NSWindow *window = self.window;
    NSSize currentSize = [window.contentView frame].size;
    NSRect frame = window.frame;

    // Calculate border around content region of the frame
    int borderX = (int)(frame.size.width - currentSize.width);
    int borderY = (int)(frame.size.height - currentSize.height);

    // Make sure the frame is smaller than the screen
    NSSize maxSize = window.screen.visibleFrame.size;

    // if we are not Scale To Screen, put an 10% of visible screen on the window
    maxSize.width = maxSize.width * 0.90;
    maxSize.height = maxSize.height * 0.90;

    // Set the new frame size
    // Add the border to the new frame size so that the content region
    // of the frame is large enough to accomodate the preview image
    frame.size.width = viewSize.width + borderX;
    frame.size.height = viewSize.height + borderY;

    // compare frame to max size of screen
    if (frame.size.width > maxSize.width)
    {
        frame.size.width = maxSize.width;
    }
    if (frame.size.height > maxSize.height)
    {
        frame.size.height = maxSize.height;
    }

    // Since upon launch we can open up the preview window if it was open
    // the last time we quit (and at the size it was) we want to make
    // sure that upon resize we do not have the window off the screen
    // So check the origin against the screen origin and adjust if
    // necessary.
    NSSize screenSize = window.screen.visibleFrame.size;
    NSPoint screenOrigin = window.screen.visibleFrame.origin;

    frame.origin.x = self.windowCenterPoint.x - floor(frame.size.width / 2);
    frame.origin.y = self.windowCenterPoint.y - floor(frame.size.height / 2);

    // our origin is off the screen to the left
    if (frame.origin.x < screenOrigin.x)
    {
        // so shift our origin to the right
        frame.origin.x = screenOrigin.x;
    }
    else if ((frame.origin.x + frame.size.width) > (screenOrigin.x + screenSize.width))
    {
        // the right side of the preview is off the screen, so shift to the left
        frame.origin.x = (screenOrigin.x + screenSize.width) - frame.size.width;
    }

    [window setFrame:frame display:YES animate:performAnimation];
}

- (void)updateSizeLabels
{
    if (self.generator)
    {
        CGFloat scale = self.previewView.scale;

        NSMutableString *scaleString = [NSMutableString string];
        if (scale * 100.0 != 100)
        {
            [scaleString appendFormat:NSLocalizedString(@"(%.0f%% actual size)", nil), scale * 100.0];
        }
        else
        {
            [scaleString appendString:NSLocalizedString(@"(Actual size)", nil)];
        }

        if (self.previewView.fitToView == YES)
        {
            [scaleString appendString:NSLocalizedString(@" Scaled To Screen", nil)];
        }

        // Set the info fields in the hud controller
        fInfoField.stringValue = self.generator.info;
        fscaleInfoField.stringValue = scaleString;

        // Set the info field in the window title bar
        self.window.title = [NSString stringWithFormat:NSLocalizedString(@"Preview - %@ %@", nil),
                             self.generator.info, scaleString];
    }
}

#pragma mark - Hud mode

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
                // Stop playback and remove the observers
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

            // Install movie notifications
            [self addMovieObservers];
        }
            break;

        default:
            break;
    }

    self.currentViewMode = mode;
}

#pragma mark -
#pragma mark Hud Control Overlay

- (void)mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];
    NSPoint mouseLoc = [theEvent locationInWindow];

    /* Test for mouse location to show/hide hud controls */
    if (self.currentViewMode != ViewModeEncoding && self.generator)
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

- (void)hideHud
{
    [fPictureControlBox setHidden:YES];
    [fMoviePlaybackControlBox setHidden:YES];
    [fEncodingControlBox setHidden:YES];
}

- (void)startHudTimer
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

- (void)stopHudTimer
{
    [self.hudTimer invalidate];
    self.hudTimer = nil;
}

- (void)hudTimerFired: (NSTimer *)theTimer
{
    // Regardless which control box is active, after the timer
    // period we want either one to fade to hidden.
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
- (void)displayPreviewAtIndex:(NSUInteger)idx
{
    if (self.window.isVisible)
    {
        CGImageRef fPreviewImage = [self.generator copyImageAtIndex:idx shouldCache:YES];
        [self.previewView setImage:fPreviewImage];
        CFRelease(fPreviewImage);
    }

    if (self.previewView.fitToView == NO && !(self.window.styleMask & NSFullScreenWindowMask))
    {
        // Get the optimal view size for the image
        NSSize imageScaledSize = [self.generator imageSize];

        // Scale the window to the image size
        NSSize windowSize = [self.previewView optimalViewSizeForImageSize:imageScaledSize minSize:NSMakeSize(MIN_WIDTH, MIN_HEIGHT)];
        [self resizeWindowForViewSize:windowSize animate:self.window.isVisible];
    }

    [self updateSizeLabels];
}

- (IBAction)previewDurationPopUpChanged:(id)sender
{
    [[NSUserDefaults standardUserDefaults] setObject:[fPreviewMovieLengthPopUp titleOfSelectedItem] forKey:@"PreviewLength"];
}

- (IBAction)pictureSliderChanged:(id)sender
{
    if ((self.pictureIndex != [fPictureSlider intValue] || !sender) && self.generator) {
        self.pictureIndex = [fPictureSlider intValue];
        [self displayPreviewAtIndex:self.pictureIndex];
    }
}

- (IBAction)toggleScaleToScreen:(id)sender
{
    if (self.previewView.fitToView == YES)
    {
        self.previewView.fitToView = NO;
        fScaleToScreenToggleButton.title = NSLocalizedString(@"Scale To Screen", nil);

        [self displayPreviewAtIndex:self.pictureIndex];
    }
    else
    {
        self.previewView.fitToView = YES;
        if (!(self.window.styleMask & NSFullScreenWindowMask))
        {
            [self.window setFrame:self.window.screen.visibleFrame display:YES animate:YES];
        }
        fScaleToScreenToggleButton.title = NSLocalizedString(@"Actual Scale", nil);
    }
}

- (IBAction) showPictureSettings: (id) sender
{
    if (self.pictureSettingsWindow == nil)
    {
        self.pictureSettingsWindow = [[HBPictureController alloc] init];
    }

    self.pictureSettingsWindow.picture = self.picture;
    [self.pictureSettingsWindow showWindow:self];
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
                                          QTMovieAskUnresolvedDataRefsAttribute: @NO,
                                          @"QTMovieOpenForPlaybackAttribute": @YES,
                                          @"QTMovieOpenAsyncRequiredAttribute": @NO,
                                          @"QTMovieOpenAsyncOKAttribute": @NO,
                                          @"QTMovieIsSteppableAttribute": @YES,
                                          QTMovieApertureModeAttribute: QTMovieApertureModeClean};

        QTMovie *movie = [[QTMovie alloc] initWithAttributes:movieAttributes error:&outError];

		if (!movie)
        {
            NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"HandBrake can't open the preview.", nil)
                                             defaultButton:NSLocalizedString(@"Open in external player", nil)
                                           alternateButton:NSLocalizedString(@"Cancel", nil)
                                               otherButton:nil
                                 informativeTextWithFormat:NSLocalizedString(@"HandBrake can't playback this combination of video/audio/container format. Do you want to open it in an external player?", nil)];
            [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
                if (returnCode == NSModalResponseOK)
                {
                    [[NSWorkspace sharedWorkspace] openURL:fileURL];
                }
            }];
            [self switchViewToMode:ViewModePicturePreview];
		}
        else
        {
            // Scale the fMovieView to the picture player size
            [fMovieView setFrame:self.previewView.pictureFrame];

            [fMovieView setMovie:movie];
            [movie setDelegate:self];

            // get and enable subtitles
            NSArray *subtitlesArray = [movie tracksOfMediaType:QTMediaTypeSubtitle];
            if (subtitlesArray.count)
            {
                // enable the first tx3g subtitle track
                [subtitlesArray[0] setEnabled:YES];
            }
            else
            {
                // Perian subtitles
                subtitlesArray = [movie tracksOfMediaType: QTMediaTypeVideo];
                if (subtitlesArray.count >= 2)
                {
                    // track 0 should be video, other video tracks should
                    // be subtitles; force-enable the first subs track
                    [subtitlesArray[1] setEnabled:YES];
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

    if ([self.generator createMovieAsyncWithImageAtIndex:self.pictureIndex
                                       duration:[[fPreviewMovieLengthPopUp titleOfSelectedItem] intValue]])
    {
        [self switchViewToMode:ViewModeEncoding];
    }
}

- (IBAction) toggleMoviePreviewPlayPause: (id) sender
{
    // make sure a movie is even loaded up
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
    // Notification for any time the movie rate changes
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(movieRateDidChange:)
                                                 name:@"QTMovieRateDidChangeNotification"
                                               object:self.movie];
}

- (void) removeMovieObservers
{
    // Notification for any time the movie rate changes
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:@"QTMovieRateDidChangeNotification"
                                                  object:self.movie];
}

- (void) movieRateDidChange: (NSNotification *) notification
{
    if (self.movie.isPlaying)
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

- (void)scrollWheel:(NSEvent *)theEvent
{
    if (self.currentViewMode != ViewModeEncoding)
    {
        if (theEvent.deltaY < 0)
        {
            [fPictureSlider setIntegerValue:self.pictureIndex < [fPictureSlider maxValue] ? self.pictureIndex + 1 : self.pictureIndex];
            [self pictureSliderChanged:self];
        }
        else if (theEvent.deltaY > 0)
        {
            [fPictureSlider setIntegerValue:self.pictureIndex > [fPictureSlider minValue] ? self.pictureIndex - 1 : self.pictureIndex];
            [self pictureSliderChanged:self];
        }
    }
}

@end
