/* $Id: HBPreviewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewController.h"
#import "HBPreviewGenerator.h"
#import "HBCroppingController.h"

#import "HBController.h"
#import "HBPreviewView.h"

#import "HBPlayer.h"
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

@interface HBPreviewController () <NSMenuItemValidation, HBPreviewGeneratorDelegate, HBPictureHUDControllerDelegate, HBEncodingProgressHUDControllerDelegate, HBPlayerHUDControllerDelegate>

@property (nonatomic, readonly) HBPictureHUDController *pictureHUD;
@property (nonatomic, readonly) HBEncodingProgressHUDController *encodingHUD;
@property (nonatomic, readonly) HBPlayerHUDController *playerHUD;

@property (nonatomic, readwrite) NSViewController<HBHUD> *currentHUD;

@property (nonatomic) NSTimer *hudTimer;
@property (nonatomic) BOOL mouseInWindow;

@property (nonatomic) id<HBPlayer> player;

@property (nonatomic) NSPopover *croppingPopover;

@property (nonatomic) NSPoint windowCenterPoint;
@property (nonatomic, weak) IBOutlet HBPreviewView *previewView;

@end

@implementation HBPreviewController

- (instancetype)init
{
    self = [super initWithWindowNibName:@"PicturePreview"];
	return self;
}

- (void)windowDidLoad
{
    self.window.tabbingMode = NSWindowTabbingModeDisallowed;
    self.window.excludedFromWindowsMenu = YES;
    self.window.acceptsMouseMovedEvents = YES;
    self.window.contentView.wantsLayer = YES;

    // Read the window center position
    // We need the center and we can't use the
    // standard NSWindow autosave because we change
    // the window size at startup.
    NSString *centerString = [NSUserDefaults.standardUserDefaults stringForKey:@"HBPreviewWindowCenter"];
    if (centerString.length)
    {
        NSPoint center = NSPointFromString(centerString);
        self.windowCenterPoint = center;
        [self.window HB_resizeToBestSizeForViewSize:NSMakeSize(MIN_WIDTH, MIN_HEIGHT) keepInScreenRect:YES centerPoint:center animate:NO];
    }
    else
    {
        self.windowCenterPoint = [self.window HB_centerPoint];
    }

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
    [self.croppingPopover close];
    self.croppingPopover = nil;
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
        self.pictureHUD.generator = generator;
    }
    else
    {
        self.previewView.image = nil;
        self.window.title = NSLocalizedString(@"Preview", @"Preview -> window title");
        self.pictureHUD.generator = nil;
    }

    [self switchStateToHUD:self.pictureHUD];

    if (generator)
    {
        [self resizeIfNeeded:NO];
    }
}

- (void)reloadPreviews
{
    if (self.generator)
    {
        [self.generator cancel];
        [self switchStateToHUD:self.pictureHUD];
        [self resizeIfNeeded:NO];
    }
}

- (void)showWindow:(id)sender
{
    [super showWindow:sender];

    if (self.currentHUD == self.pictureHUD)
    {
        [self reloadPreviews];
    }

    if (self.generator)
    {
        [self showHud:self.currentHUD];
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

    [self.generator purgeImageCache];
}

#pragma mark - Window sizing

- (void)resizeIfNeeded:(BOOL)forceResize
{
    if (!(self.window.styleMask & NSWindowStyleMaskFullScreen))
    {
        if (self.previewView.fitToView)
        {
            [self.window setFrame:self.window.screen.visibleFrame display:YES animate:YES];
        }
        else
        {
            // Get the optimal view size for the image
            NSSize windowSize = [self.previewView optimalViewSizeForImageSize:self.generator.imageSize
                                                                      minSize:NSMakeSize(MIN_WIDTH, MIN_HEIGHT)
                                                                  scaleFactor:self.window.backingScaleFactor];

            if (forceResize ||
                windowSize.width  > self.window.contentView.frame.size.width ||
                windowSize.height > self.window.contentView.frame.size.height)
            {
                // Scale the window to the image size
                [self.window HB_resizeToBestSizeForViewSize:windowSize keepInScreenRect:YES centerPoint:NSZeroPoint animate:self.window.isVisible];
            }
        }
    }

    [self updateSizeLabels];
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
    NSWindow *theWindow = (NSWindow *)notification.object;

    CGFloat newBackingScaleFactor = theWindow.backingScaleFactor;
    CGFloat oldBackingScaleFactor = [notification.userInfo[NSBackingPropertyOldScaleFactorKey] doubleValue];

    if (newBackingScaleFactor != oldBackingScaleFactor)
    {
        // Scale factor changed, resize the preview window
        if (self.generator)
        {
            [self resizeIfNeeded:NO];
        }
    }
}

#pragma mark - Window sizing

- (void)windowDidMove:(NSNotification *)notification
{
    if (self.previewView.fitToView == NO)
    {
        self.windowCenterPoint = [self.window HB_centerPoint];
        [NSUserDefaults.standardUserDefaults setObject:NSStringFromPoint(self.windowCenterPoint) forKey:@"HBPreviewWindowCenter"];
    }
}

- (void)windowDidResize:(NSNotification *)notification
{
    [self updateSizeLabels];
    if (self.currentHUD == self.playerHUD)
    {
        [CATransaction begin];
        CATransaction.disableActions = YES;
        self.player.layer.frame = self.previewView.pictureFrame;
        [CATransaction commit];
    }
}

- (void)updateSizeLabels
{
    if (self.generator)
    {
        CGFloat scale = self.previewView.scale;

        NSMutableString *scaleString = [NSMutableString string];
        if (scale * 100.0 != 100)
        {
            [scaleString appendFormat:NSLocalizedString(@"(%.0f%% actual size)", @"Preview -> size info label"), floor(scale * 100.0)];
        }
        else
        {
            [scaleString appendString:NSLocalizedString(@"(Actual size)", @"Preview -> size info label")];
        }

        if (self.previewView.fitToView == YES)
        {
            [scaleString appendString:NSLocalizedString(@" Scaled To Screen", @"Preview -> size info label")];
        }

        // Set the info fields in the hud controller
        self.pictureHUD.info = self.generator.info;
        self.pictureHUD.scale = scaleString;

        // Set the info field in the window title bar
        self.window.title = [NSString stringWithFormat:NSLocalizedString(@"Preview - %@ %@", @"Preview -> window title format"),
                             self.generator.info, scaleString];
    }
}

- (void)setScaleToScreen:(BOOL)scaleToScreen
{
    self.previewView.fitToView = scaleToScreen;
    [self resizeIfNeeded:YES];
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

    if (self.generator && self.currentHUD != hud)
    {
        [self showHud:hud];
    }

    self.currentHUD = hud;
}

#pragma mark - HUD Control Overlay

- (void)mouseEntered:(NSEvent *)theEvent
{
    if (self.generator)
    {
        [self showHudWithAnimation:self.currentHUD];
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
        NSPoint mouseLoc = theEvent.locationInWindow;

        if (NSPointInRect(mouseLoc, self.currentHUD.view.frame))
        {
            [self stopHudTimer];
        }
        else
        {
            [self showHudWithAnimation:self.currentHUD];
            [self startHudTimer];
        }
	}
}

- (void)showHud:(NSViewController<HBHUD> *)HUD
{
    NSMutableArray<NSViewController<HBHUD> *> *HUDs = [@[self.pictureHUD, self.encodingHUD, self.playerHUD] mutableCopy];
    [HUDs removeObject:HUD];
    for (NSViewController *controller in HUDs)
    {
        controller.view.hidden = YES;
    }

    HUD.view.hidden = NO;
    HUD.view.layer.opacity = 1.0;

    [self.window makeFirstResponder:HUD.view];
    [self startHudTimer];
}

- (void)showHudWithAnimation:(NSViewController<HBHUD> *)HUD
{
    // The standard view animator doesn't play
    // nicely with the Yosemite visual effects yet.
    // So let's do the fade ourself.
    NSView *view = HUD.view;
    CALayer *layer = view.layer;

    if (layer.opacity == 0 || view.isHidden)
    {
        view.hidden = NO;

        [CATransaction begin];
        CABasicAnimation *fadeInAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        fadeInAnimation.fromValue = @([layer.presentationLayer opacity]);
        fadeInAnimation.toValue = @1.0;
        fadeInAnimation.beginTime = 0.0;
        fadeInAnimation.duration = ANIMATION_DUR;

        [layer addAnimation:fadeInAnimation forKey:nil];
        [layer setOpacity:1];

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
        fadeOutAnimation.toValue = @0.0;
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

- (void)displayPreviewAtIndex:(NSUInteger)idx
{
    if (self.generator && self.window.isVisible)
    {
        CGImageRef image = [self.generator copyImageAtIndex:idx shouldCache:YES];
        if (image)
        {
            self.previewView.image = image;
            CFRelease(image);
        }
    }
}

- (void)showCroppingSettings:(id)sender
{
    if (self.croppingPopover)
    {
        if (self.croppingPopover.isShown)
        {
            [self.croppingPopover close];
        }
        else
        {
            [self.croppingPopover showRelativeToRect:[sender bounds] ofView:sender preferredEdge:NSMaxYEdge];
        }
    }
    else
    {
        HBCroppingController *croppingController = [[HBCroppingController alloc] initWithPicture:self.picture];
        self.croppingPopover = [[NSPopover alloc] init];
        self.croppingPopover.behavior = NSPopoverBehaviorTransient;
        self.croppingPopover.contentViewController = croppingController;
        self.croppingPopover.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
        [self.croppingPopover showRelativeToRect:[sender bounds] ofView:sender preferredEdge:NSMaxYEdge];
    }
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

- (void)showAlert:(NSURL *)fileURL
{
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = NSLocalizedString(@"HandBrake can't open the preview.", @"Preview -> live preview alert message");
    alert.informativeText = NSLocalizedString(@"HandBrake can't playback this combination of video/audio/container format. Do you want to open it in an external player?", @"Preview -> live preview alert informative text");
    [alert addButtonWithTitle:NSLocalizedString(@"Open in external player", @"Preview -> live preview alert default button")];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"Preview -> live preview alert alternate button")];

    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode)
    {
        if (returnCode == NSAlertFirstButtonReturn)
        {
            [[NSWorkspace sharedWorkspace] openURL:fileURL];
        }
    }];
}

- (void)setUpPlaybackOfURL:(NSURL *)fileURL playerClass:(Class)class
{
    NSArray<Class> *availablePlayerClasses = @[[HBAVPlayer class]];

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

    [self.previewView.layer insertSublayer:playerLayer atIndex:10];
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
    if (self.generator && [self.currentHUD HB_keyDown:event] == NO)
    {
        [super keyDown:event];
    }
}

- (void)scrollWheel:(NSEvent *)event
{
    if (self.generator && [self.currentHUD HB_scrollWheel:event] == NO)
    {
        [super scrollWheel:event];
    }
}

@end
