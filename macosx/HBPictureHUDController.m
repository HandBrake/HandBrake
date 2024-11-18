/*  HBPictureHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPictureHUDController.h"

@interface HBPictureHUDController ()

@property (nonatomic, weak) IBOutlet NSTextField *scaleLabel;
@property (nonatomic, weak) IBOutlet NSTextField *infoLabel;

@property (nonatomic, weak) IBOutlet NSSlider *slider;

@property (nonatomic, weak) IBOutlet NSPopUpButton *durationPopUp;
@property (nonatomic, weak) IBOutlet NSButton *scaleToScreenButton;

@property (nonatomic, weak) IBOutlet NSTextField *durationLabel;
@property (nonatomic, weak) IBOutlet NSTextField *durationUnitLabel;

@property (nonatomic) BOOL fitToView;

@end

@interface HBPictureHUDController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
- (void)_touchBar_updateFitToView:(BOOL)fitToView;
- (void)_touchBar_validateUserInterfaceItems;
@end


@implementation HBPictureHUDController

- (NSString *)nibName
{
    return @"HBPictureHUDController";
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // we set the preview length popup in seconds
    [self.durationPopUp removeAllItems];
    [self.durationPopUp addItemsWithTitles:@[@"5", @"15", @"30", @"45", @"60", @"90",
                                             @"120", @"150", @"180", @"210", @"240"]];

    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewLength"])
    {
        [self.durationPopUp selectItemWithTitle:[[NSUserDefaults standardUserDefaults]
                                                       objectForKey:@"PreviewLength"]];
    }
    if (!self.durationPopUp.selectedItem)
    {
        // currently hard set default to 15 seconds
        [self.durationPopUp selectItemAtIndex:0];
    }
}

- (BOOL)canBeHidden
{
    return YES;
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    _generator = generator;
    NSUInteger imagesCount = generator.imagesCount;

    if (imagesCount > 0)
    {
        self.slider.numberOfTickMarks = imagesCount;
        self.slider.maxValue = imagesCount - 1;

        if (self.selectedIndex > imagesCount)
        {
            self.selectedIndex = imagesCount - 1;
        }
    }

    [self _touchBar_validateUserInterfaceItems];
}

- (void)setSelectedIndex:(NSUInteger)selectedIndex
{
    _selectedIndex = selectedIndex;
    self.slider.integerValue = selectedIndex;
    [self.delegate displayPreviewAtIndex:self.selectedIndex];
}

- (void)setInfo:(NSString *)info
{
    self.infoLabel.stringValue = info;
}

- (void)setScale:(NSString *)scale
{
    self.scaleLabel.stringValue = scale;
}

- (void)setFitToView:(BOOL)fitToView
{
    _fitToView = fitToView;
    if (fitToView == NO)
    {
        self.scaleToScreenButton.title = NSLocalizedString(@"Scale To Screen", @"Picture HUD -> scale button");
    }
    else
    {
        self.scaleToScreenButton.title = NSLocalizedString(@"Actual Scale", @"Picture HUD -> scale button");
    }
    [self _touchBar_updateFitToView:fitToView];
    [self.delegate setScaleToScreen:fitToView];
}

- (BOOL)validateUserIterfaceItemForAction:(SEL)action
{
    if (action == @selector(createMoviePreview:) ||
        action == @selector(scaleToScreen:) ||
        action == @selector(actualSize:) ||
        action == @selector(toggleScaleToScreen:))
    {
        return self.generator != nil;
    }
    return YES;
}

- (IBAction)previewDurationPopUpChanged:(id)sender
{
    [[NSUserDefaults standardUserDefaults] setObject:self.durationPopUp.titleOfSelectedItem forKey:@"PreviewLength"];
}

- (IBAction)pictureSliderChanged:(id)sender
{
    NSUInteger index = self.slider.integerValue;
    self.selectedIndex = index;
}

- (IBAction)scaleToScreen:(id)sender
{
    self.fitToView = YES;
}

- (IBAction)actualSize:(id)sender
{
    self.fitToView = NO;
}

- (IBAction)toggleScaleToScreen:(id)sender
{
    self.fitToView = !self.fitToView;
}

- (IBAction)showCroppingSettings:(id)sender
{
    [self.delegate showCroppingSettings:sender];
}

- (IBAction)createMoviePreview:(id)sender
{
    [self.delegate createMoviePreviewWithPictureIndex:self.selectedIndex duration:self.durationPopUp.titleOfSelectedItem.intValue];
}

- (BOOL)HB_keyDown:(NSEvent *)event
{
    unichar key = [event.charactersIgnoringModifiers characterAtIndex:0];
    if (key == NSLeftArrowFunctionKey)
    {
        self.selectedIndex = self.selectedIndex > 0 ? self.selectedIndex - 1 : self.selectedIndex;
        return YES;
    }
    else if (key == NSRightArrowFunctionKey)
    {
        self.selectedIndex = self.selectedIndex < self.generator.imagesCount - 1 ? self.selectedIndex + 1 : self.selectedIndex;
        return YES;
    }
    else
    {
        return NO;
    }
}

- (BOOL)HB_scrollWheel:(NSEvent *)theEvent
{
    if (theEvent.deltaY < 0)
    {
        self.selectedIndex = self.selectedIndex < self.generator.imagesCount - 1 ? self.selectedIndex + 1 : self.selectedIndex;
    }
    else if (theEvent.deltaY > 0)
    {
        self.selectedIndex = self.selectedIndex > 0 ? self.selectedIndex - 1 : self.selectedIndex;
    }
    return YES;
}

@end

@implementation HBPictureHUDController (TouchBar)

#pragma mark - NSTouchBar

static NSTouchBarItemIdentifier HBTouchBarMain = @"fr.handbrake.previewWindowTouchBar";

static NSTouchBarItemIdentifier HBTouchBarRip = @"fr.handbrake.rip";
static NSTouchBarItemIdentifier HBTouchBarFitToScreen = @"fr.handbrake.fitToScreen";

@dynamic touchBar;

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[HBTouchBarRip, HBTouchBarFitToScreen, NSTouchBarItemIdentifierFlexibleSpace];

    bar.customizationIdentifier = HBTouchBarMain;
    bar.customizationAllowedItemIdentifiers = @[HBTouchBarRip, HBTouchBarFitToScreen, NSTouchBarItemIdentifierFlexibleSpace];

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarRip])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Live Preview", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarPlayTemplate]
                                              target:self action:@selector(createMoviePreview:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarFitToScreen])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Scale To Screen", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarEnterFullScreenTemplate]
                                              target:self action:@selector(toggleScaleToScreen:)];

        item.view = button;
        return item;
    }

    return nil;
}

- (void)_touchBar_updateFitToView:(BOOL)fitToView
{
    NSButton *button = (NSButton *)[[self.touchBar itemForIdentifier:HBTouchBarFitToScreen] view];
    if (fitToView == NO)
    {
        button.image = [NSImage imageNamed:NSImageNameTouchBarEnterFullScreenTemplate];
    }
    else
    {
        button.image = [NSImage imageNamed:NSImageNameTouchBarExitFullScreenTemplate];
    }
}

- (void)_touchBar_validateUserInterfaceItems
{
    for (NSTouchBarItemIdentifier identifier in self.touchBar.itemIdentifiers) {
        NSTouchBarItem *item = [self.touchBar itemForIdentifier:identifier];
        NSView *view = item.view;
        if ([view isKindOfClass:[NSButton class]]) {
            NSButton *button = (NSButton *)view;
            BOOL enabled = [self validateUserIterfaceItemForAction:button.action];
            button.enabled = enabled;
        }
    }
}

@end
