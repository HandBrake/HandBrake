/*  HBPreviewViewController.m

This file is part of the HandBrake source code.
Homepage: <http://handbrake.fr/>.
It may be used under the terms of the GNU General Public License. */

#import <QuartzCore/QuartzCore.h>
#import "HBPreviewViewController.h"

#import "HBPreviewView.h"
#import "HBPreviewGenerator.h"
#import "HBPreviewController.h"

@interface HBPreviewViewController ()

@property (nonatomic, strong) IBOutlet NSView *hud;
@property (nonatomic, strong) IBOutlet HBPreviewView *previewView;

@property (nonatomic) NSInteger selectedIndex;

@property (nonatomic) BOOL visible;
@property (nonatomic) BOOL mouseInView;
@property (nonatomic) BOOL wantsDisplayLayer;

@end

@implementation HBPreviewViewController

- (instancetype)init
{
    self = [super initWithNibName:@"HBPreviewViewController" bundle:nil];
    if (self)
    {
        _selectedIndex = 1;
        _wantsDisplayLayer = NO;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.visible = YES;
    self.previewView.showShadow = NO;

    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:self.view.frame
                                                                options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
                                                                  owner:self
                                                               userInfo:nil];
    [self.view addTrackingArea:trackingArea];
    self.hud.hidden = YES;
    self.hud.layer.opacity = 0;
}

- (void)viewWillAppear
{
    self.visible = YES;
    [self updatePicture];
}

- (void)viewDidDisappear
{
    self.visible = NO;
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    _generator = generator;
    if (generator)
    {
        self.selectedIndex = self.selectedIndex;
    }
    [self updatePicture];
}

- (void)update
{
    [self updatePicture];
}

#pragma mark - HUD

- (void)mouseEntered:(NSEvent *)theEvent
{
    if (self.generator)
    {
        [self showHudWithAnimation:self.hud];
    }
    self.mouseInView = YES;
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];

    // Test for mouse location to show/hide hud controls
    if (self.generator && self.mouseInView)
    {
        [self showHudWithAnimation:self.hud];
    }
}

- (void)mouseExited:(NSEvent *)theEvent
{
    [self hideHudWithAnimation:self.hud];
    self.mouseInView = NO;
}

#define ANIMATION_DUR 0.15

- (void)showHudWithAnimation:(NSView *)hud
{
    // The standard view animator doesn't play
    // nicely with the Yosemite visual effects yet.
    // So let's do the fade ourself.
    if (hud.layer.opacity == 0 || hud.isHidden)
    {
        hud.hidden = NO;

        [CATransaction begin];
        CABasicAnimation *fadeInAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        fadeInAnimation.fromValue = @(hud.layer.presentationLayer.opacity);
        fadeInAnimation.toValue = @1.0;
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
        fadeOutAnimation.fromValue = @(hud.layer.presentationLayer.opacity);
        fadeOutAnimation.toValue = @0.0;
        fadeOutAnimation.beginTime = 0.0;
        fadeOutAnimation.duration = ANIMATION_DUR;

        [hud.layer addAnimation:fadeOutAnimation forKey:nil];
        [hud.layer setOpacity:0];

        [CATransaction commit];
    }
}

#pragma mark - Preview index

- (void)setSelectedIndex:(NSInteger)selectedIndex
{
    NSInteger count = self.generator.imagesCount;
    if (selectedIndex >= count)
    {
        selectedIndex = count -1;
    }
    else if (selectedIndex < 0)
    {
        selectedIndex = 0;
    }
    _selectedIndex = selectedIndex;
}

- (IBAction)next:(id)sender
{
    self.selectedIndex += 1;
    [self updatePicture];
}

- (IBAction)previous:(id)sender
{
    self.selectedIndex -= 1;
    [self updatePicture];
}

- (void)updatePicture
{
    if (self.generator && self.visible)
    {
        CFTypeRef image = self.wantsDisplayLayer ?
                        (CFTypeRef)[self.generator copyPixelBufferAtIndex:self.selectedIndex shouldCache:NO] :
                        (CFTypeRef)[self.generator copyImageAtIndex:self.selectedIndex shouldCache:YES];
        if (image)
        {
            self.previewView.image = (__bridge id _Nullable)(image);
            CFRelease(image);
            self.previewView.layer.opacity = 1;
        }
    }
    else
    {
        self.previewView.image = [NSImage imageNamed:@"ColorBars"];
        self.previewView.layer.opacity = .3;
    }
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    if (theEvent.deltaY < 0)
    {
        self.selectedIndex += 1;
        [self updatePicture];
    }
    else if (theEvent.deltaY > 0)
    {
        self.selectedIndex -= 1;
        [self updatePicture];
    }
}

@end
