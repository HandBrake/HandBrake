/* $Id: HBPreviewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewController.h"
#import "HBPreviewGenerator.h"
#import "HBPictureController.h"

#import "HBController.h"
#import "HBPreviewView.h"

#import "HBPlayer.h"
#import "HBQTKitPlayer.h"
#import "HBAVPlayer.h"

#import "HBPictureHUDController.h"
#import "HBEncodingProgressHUDController.h"
#import "HBPlayerHUDController.h"

#import "NSWindow+HBAdditions.h"

#define ANIMATION_DUR 0.15
#define HUD_FADEOUT_TIME 4.0

// Make min width and height of preview window large enough for hud.
#define MIN_WIDTH 480.0
#define MIN_HEIGHT 360.0

@interface HBPreviewController () <HBPreviewGeneratorDelegate, HBPictureHUDControllerDelegate, HBEncodingProgressHUDControllerDelegate, HBPlayerHUDControllerDelegate>

@property (nonatomic, readonly) HBPictureHUDController *pictureHUD;
@property (nonatomic, readonly) HBEncodingProgressHUDController *encodingHUD;
@property (nonatomic, readonly) HBPlayerHUDController *playerHUD;

@property (nonatomic, readwrite) NSViewController<HBHUD> *currentHUD;

@property (nonatomic) NSTimer *hudTimer;
@property (nonatomic) BOOL mouseInWindow;

@property (nonatomic) id<HBPlayer> player;

@property (nonatomic) HBPictureController *pictureSettingsWindow;

@property (nonatomic) NSPoint windowCenterPoint;
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
        self.windowCenterPoint = center;
        [self.window HB_resizeToBestSizeForViewSize:NSMakeSize(MIN_WIDTH, MIN_HEIGHT) center:self.windowCenterPoint animate:NO];
    }
    else
    {
        self.windowCenterPoint = [self.window HB_centerPoint];
    }

    self.window.excludedFromWindowsMenu = YES;
    self.window.acceptsMouseMovedEvents = YES;

    _pictureHUD = [[HBPictureHUDController alloc] init];
    self.pictureHUD.delegate = self;
    _encodingHUD = [[HBEncodingProgressHUDController alloc] init];
    self.encodingHUD.delegate = self;
    _playerHUD = [[HBPlayerHUDController alloc] init];
    self.playerHUD.delegate = self;

    [self.window.contentView addSubview:self.pictureHUD.view];
    [self.window.contentView addSubview:self.encodingHUD.view];
    [self.window.contentView addSubview:self.playerHUD.view];

    // Relocate our hud origins.
    CGPoint origin = CGPointMake(floor((self.window.frame.size.width - _pictureHUD.view.bounds.size.width) / 2), MIN_HEIGHT / 10);

    [self.pictureHUD.view setFrameOrigin:origin];
    [self.encodingHUD.view setFrameOrigin:origin];
    [self.playerHUD.view setFrameOrigin:origin];

    self.currentHUD = self.pictureHUD;
    self.currentHUD.view.hidden = YES;

    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:self.window.contentView.frame
                                                                 options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
                                                                   owner:self
                                                                userInfo:nil];
    [self.window.contentView addTrackingArea:trackingArea];
}

- (void)dealloc
{
    [_hudTimer invalidate];
    _generator.delegate = nil;
    [_generator cancel];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(selectPresetFromMenu:))
    {
        return [self.documentController validateMenuItem:menuItem];
    }

    return YES;
}

- (IBAction)selectDefaultPreset:(id)sender
{
    [self.documentController selectDefaultPreset:sender];
}

- (NSUndoManager *)windowWillReturnUndoManager:(NSWindow *)window
{
    return self.documentController.window.undoManager;
}

- (IBAction)selectPresetFromMenu:(id)sender
{
    [self.documentController selectPresetFromMenu:sender];
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
        self.pictureHUD.pictureCount = generator.imagesCount;
    }
    else
    {
        self.previewView.image = nil;
        self.window.title = NSLocalizedString(@"Preview", nil);
    }
    [self switchStateToHUD:self.pictureHUD];
}

- (void)reloadPreviews
{
    if (self.generator)
    {
        [self.generator cancel];
        [self switchStateToHUD:self.pictureHUD];
    }
}

- (void)showWindow:(id)sender
{
    [super showWindow:sender];

    if (self.currentHUD == self.pictureHUD)
    {
        [self reloadPreviews];
    }
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    if (self.currentHUD == self.encodingHUD)
    {
        [self cancelEncoding];
    }
    else if (self.currentHUD == self.playerHUD)
    {
        [self.player pause];
    }

    [self.pictureSettingsWindow close];
    [self.generator purgeImageCache];
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
    NSWindow *theWindow = (NSWindow *)notification.object;

    CGFloat newBackingScaleFactor = theWindow.backingScaleFactor;
    CGFloat oldBackingScaleFactor = [notification.userInfo[@"NSBackingPropertyOldScaleFactorKey"] doubleValue];

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

- (void)windowDidMove:(NSNotification *)notification
{
    if (self.previewView.fitToView == NO)
    {
        self.windowCenterPoint = [self.window HB_centerPoint];
        [[NSUserDefaults standardUserDefaults] setObject:NSStringFromPoint(self.windowCenterPoint) forKey:@"HBPreviewWindowCenter"];
    }
}

- (void)windowDidResize:(NSNotification *)notification
{
    [self updateSizeLabels];
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
        self.pictureHUD.info = self.generator.info;
        self.pictureHUD.scale = scaleString;

        // Set the info field in the window title bar
        self.window.title = [NSString stringWithFormat:NSLocalizedString(@"Preview - %@ %@", nil),
                             self.generator.info, scaleString];
    }
}

#pragma mark - Hud State

/**
 * Switch the preview controller to one of his hud mode:
 * This methods is the only way to change the mode, do not try otherwise.
 * @param hud NSViewController<HBHUD> the hud to show
 */
- (void)switchStateToHUD:(NSViewController<HBHUD> *)hud
{
    if (self.currentHUD == self.playerHUD)
    {
        [self exitPlayerState];
    }

    if (hud == self.pictureHUD)
    {
        [self enterPictureState];
    }
    else if (hud == self.encodingHUD)
    {
        [self enterEncodingState];
    }
    else if (hud == self.playerHUD)
    {
        [self enterPlayerState];
    }

    // Show the current hud
    NSMutableArray *huds = [@[self.pictureHUD, self.encodingHUD, self.playerHUD] mutableCopy];
    [huds removeObject:hud];
    for (NSViewController *controller in huds) {
        controller.view.hidden = YES;
    }
    if (self.generator)
    {
        hud.view.hidden = NO;
        hud.view.layer.opacity = 1.0;
    };

    [self.window makeFirstResponder:hud.view];
    [self startHudTimer];
    self.currentHUD = hud;
}

#pragma mark - HUD Control Overlay

- (void)mouseEntered:(NSEvent *)theEvent
{
    if (self.generator)
    {
        NSView *hud = self.currentHUD.view;

        [self showHudWithAnimation:hud];
        [self startHudTimer];
    }
    self.mouseInWindow = YES;
}

- (void)mouseExited:(NSEvent *)theEvent
{
    [self hudTimerFired:nil];
    self.mouseInWindow = NO;
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];

    // Test for mouse location to show/hide hud controls
    if (self.generator && self.mouseInWindow)
    {
        NSView *hud = self.currentHUD.view;
        NSPoint mouseLoc = theEvent.locationInWindow;

        if (NSPointInRect(mouseLoc, hud.frame))
        {
            [self stopHudTimer];
        }
        else
        {
            [self showHudWithAnimation:hud];
            [self startHudTimer];
        }
	}
}

- (void)showHudWithAnimation:(NSView *)hud
{
    // The standard view animator doesn't play
    // nicely with the Yosemite visual effects yet.
    // So let's do the fade ourself.
    if (hud.layer.opacity == 0 || hud.isHidden)
    {
        [hud setHidden:NO];

        [CATransaction begin];
        CABasicAnimation *fadeInAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        fadeInAnimation.fromValue = @([hud.layer.presentationLayer opacity]);
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
        CABasicAnimation *fadeOutAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        fadeOutAnimation.fromValue = @([hud.layer.presentationLayer opacity]);
        fadeOutAnimation.toValue = @(0.0);
        fadeOutAnimation.beginTime = 0.0;
        fadeOutAnimation.duration = ANIMATION_DUR;

        [hud.layer addAnimation:fadeOutAnimation forKey:nil];
        [hud.layer setOpacity:0];

        [CATransaction commit];
    }
}

- (void)startHudTimer
{
	if (self.hudTimer)
    {
        [self.hudTimer setFireDate:[NSDate dateWithTimeIntervalSinceNow:HUD_FADEOUT_TIME]];
	}
    else
    {
        self.hudTimer = [NSTimer scheduledTimerWithTimeInterval:HUD_FADEOUT_TIME target:self selector:@selector(hudTimerFired:)
                                                       userInfo:nil repeats:YES];
    }
}

- (void)stopHudTimer
{
    [self.hudTimer invalidate];
    self.hudTimer = nil;
}

- (void)hudTimerFired:(NSTimer *)theTimer
{
    if (self.currentHUD.canBeHidden)
    {
        [self hideHudWithAnimation:self.currentHUD.view];
    }
    [self stopHudTimer];
}

#pragma mark - Still previews mode

- (void)enterPictureState
{
    [self displayPreviewAtIndex:self.pictureHUD.selectedIndex];
}

/**
 * Adjusts the window to draw the current picture (fPicture) adjusting its size as
 * necessary to display as much of the picture as possible.
 */
- (void)displayPreviewAtIndex:(NSUInteger)idx
{
    if (!self.generator)
    {
        return;
    }

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
        [self.window HB_resizeToBestSizeForViewSize:windowSize center:self.windowCenterPoint animate:self.window.isVisible];
    }

    [self updateSizeLabels];
}

- (void)toggleScaleToScreen
{
    if (self.previewView.fitToView == YES)
    {
        self.previewView.fitToView = NO;
        [self displayPreviewAtIndex:self.pictureHUD.selectedIndex];
    }
    else
    {
        self.previewView.fitToView = YES;
        if (!(self.window.styleMask & NSFullScreenWindowMask))
        {
            [self.window setFrame:self.window.screen.visibleFrame display:YES animate:YES];
        }
    }
}

- (void)showPictureSettings
{
    if (self.pictureSettingsWindow == nil)
    {
        self.pictureSettingsWindow = [[HBPictureController alloc] init];
        self.pictureSettingsWindow.previewController = self;
    }

    self.pictureSettingsWindow.picture = self.picture;
    [self.pictureSettingsWindow showWindow:self];
}

#pragma mark - Encoding mode

- (void)enterEncodingState
{
    self.encodingHUD.progress = 0;
}

- (void)cancelEncoding
{
    [self.generator cancel];
}

- (void)createMoviePreviewWithPictureIndex:(NSUInteger)index duration:(NSUInteger)duration
{
    if ([self.generator createMovieAsyncWithImageAtIndex:index duration:duration])
    {
        [self switchStateToHUD:self.encodingHUD];
    }
}

- (void)updateProgress:(double)progress info:(NSString *)progressInfo
{
    self.encodingHUD.progress = progress;
    self.encodingHUD.info = progressInfo;
}

- (void)didCancelMovieCreation
{
    [self switchStateToHUD:self.pictureHUD];
}

- (void)showAlert:(NSURL *)fileURL;
{
    NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"HandBrake can't open the preview.", nil)
                                     defaultButton:NSLocalizedString(@"Open in external player", nil)
                                   alternateButton:NSLocalizedString(@"Cancel", nil)
                                       otherButton:nil
                         informativeTextWithFormat:NSLocalizedString(@"HandBrake can't playback this combination of video/audio/container format. Do you want to open it in an external player?", nil)];

    [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:(void *)CFBridgingRetain(fileURL)];
}

- (void)alertDidEnd:(NSAlert *)alert
         returnCode:(NSInteger)returnCode
        contextInfo:(void *)contextInfo
{
    NSURL *fileURL = CFBridgingRelease(contextInfo);
    if (returnCode == NSModalResponseOK)
    {
        [[NSWorkspace sharedWorkspace] openURL:fileURL];
    }
}

- (void)setUpPlaybackOfURL:(NSURL *)fileURL playerClass:(Class)class;
{
#if __HB_QTKIT_PLAYER_AVAILABLE
    NSArray<Class> *availablePlayerClasses = @[[HBAVPlayer class], [HBQTKitPlayer class]];
#else
    NSArray<Class> *availablePlayerClasses = @[[HBAVPlayer class]];
#endif

    self.player = [[class alloc] initWithURL:fileURL];

    if (self.player)
    {
        [self.player loadPlayableValueAsynchronouslyWithCompletionHandler:^{

            dispatch_async(dispatch_get_main_queue(), ^{
                if (self.player.isPlayable && self.currentHUD == self.encodingHUD)
                {
                    [self switchStateToHUD:self.playerHUD];
                }
                else
                {
                    // Try to open the preview with the next player class.
                    NSUInteger idx = [availablePlayerClasses indexOfObject:class];
                    if (idx != NSNotFound && (idx + 1) < availablePlayerClasses.count)
                    {
                        Class nextPlayer = availablePlayerClasses[idx + 1];
                        [self setUpPlaybackOfURL:fileURL playerClass:nextPlayer];
                    }
                    else
                    {
                        [self showAlert:fileURL];
                        [self switchStateToHUD:self.pictureHUD];
                    }
                }
            });

        }];
    }
    else
    {
        [self showAlert:fileURL];
        [self switchStateToHUD:self.pictureHUD];
    }
}

- (void)didCreateMovieAtURL:(NSURL *)fileURL
{
    [self setUpPlaybackOfURL:fileURL playerClass:[HBAVPlayer class]];
}

#pragma mark - Player mode

- (void)enterPlayerState
{
    // Scale the layer to the picture player size
    CALayer *playerLayer = self.player.layer;
    playerLayer.frame = self.previewView.pictureFrame;

    [self.window.contentView.layer insertSublayer:playerLayer atIndex:1];

    self.playerHUD.player = self.player;
}

- (void)exitPlayerState
{
    self.playerHUD.player = nil;
    [self.player pause];
    [self.player.layer removeFromSuperlayer];
    self.player = nil;
}

- (void)stopPlayer
{
    [self switchStateToHUD:self.pictureHUD];
}

#pragma mark - Scroll

- (void)keyDown:(NSEvent *)event
{
    if ([self.currentHUD HB_keyDown:event] == NO)
    {
        [super keyDown:event];
    }
}

- (void)scrollWheel:(NSEvent *)event
{
    if ([self.currentHUD HB_scrollWheel:event] == NO)
    {
        [super scrollWheel:event];
    }
}

@end
