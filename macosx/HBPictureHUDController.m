/*  HBPictureHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPictureHUDController.h"

#import "HBThumbnailItemView.h"

@interface HBPictureHUDController ()

@property (nonatomic, weak) IBOutlet NSTextField *scaleLabel;
@property (nonatomic, weak) IBOutlet NSTextField *infoLabel;

@property (nonatomic, weak) IBOutlet NSSlider *slider;

@property (nonatomic, weak) IBOutlet NSPopUpButton *durationPopUp;
@property (nonatomic, weak) IBOutlet NSButton *scaleToScreenButton;

@property (nonatomic, weak) IBOutlet NSTextField *durationLabel;
@property (nonatomic, weak) IBOutlet NSTextField *durationUnitLabel;

@property (nonatomic) BOOL fitToView;
@property (nonatomic) BOOL ignoreUpdates;

@end

@interface HBPictureHUDController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate, NSScrubberDataSource, NSScrubberDelegate>
- (void)_touchBar_reloadScrubberData;
- (void)_touchBar_updateScrubberSelectedIndex:(NSUInteger)selectedIndex;
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
    [self.durationPopUp addItemsWithTitles:@[@"15", @"30", @"45", @"60", @"90",
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

    if (@available(macOS 10.12.2, *))
    {
        [self _touchBar_reloadScrubberData];
        [self _touchBar_validateUserInterfaceItems];
    }
}

- (void)setSelectedIndex:(NSUInteger)selectedIndex
{
    _selectedIndex = selectedIndex;
    self.slider.integerValue = selectedIndex;
    if (@available(macOS 10.12.2, *))
    {
        [self _touchBar_updateScrubberSelectedIndex:selectedIndex];
    }
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
    if (@available(macOS 10.12.2, *))
    {
        [self _touchBar_updateFitToView:fitToView];
    }
}

- (BOOL)validateUserIterfaceItemForAction:(SEL)action
{
    if (action == @selector(createMoviePreview:) || action == @selector(toggleScaleToScreen:))
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

- (IBAction)toggleScaleToScreen:(id)sender
{
    [self.delegate toggleScaleToScreen];
    self.fitToView = !self.fitToView;
}

- (IBAction)showPictureSettings:(id)sender
{
    [self.delegate showPictureSettings];
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
        self.ignoreUpdates = YES;
        self.selectedIndex = self.selectedIndex > 0 ? self.selectedIndex - 1 : self.selectedIndex;
        return YES;
    }
    else if (key == NSRightArrowFunctionKey)
    {
        self.ignoreUpdates = YES;
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

#pragma mark - NSScrubberDataSource

NSString *thumbnailScrubberItemIdentifier = @"thumbnailItem";

- (NSInteger)numberOfItemsForScrubber:(NSScrubber *)scrubber
{
    return self.generator.imagesCount;
}

- (NSScrubberItemView *)scrubber:(NSScrubber *)scrubber viewForItemAtIndex:(NSInteger)index
{
    HBThumbnailItemView *itemView = [scrubber makeItemWithIdentifier:thumbnailScrubberItemIdentifier owner:nil];
    itemView.generator = self.generator;
    itemView.thumbnailIndex = index;
    return itemView;
}

#pragma mark - NSScrubberFlowLayoutDelegate

// Scrubber is asking for the size for a particular item.
- (NSSize)scrubber:(NSScrubber *)scrubber layout:(NSScrubberFlowLayout *)layout sizeForItemAtIndex:(NSInteger)itemIndex
{
    NSInteger val = 50;
    return NSMakeSize(val, 30);
}

#pragma mark - NSScrubberDelegate

- (void)scrubber:(NSScrubber *)scrubber didSelectItemAtIndex:(NSInteger)selectedIndex
{
    if (self.selectedIndex != selectedIndex && self.ignoreUpdates == NO)
    {
        self.selectedIndex = selectedIndex;
    }
    self.ignoreUpdates = NO;
}

#pragma mark - NSTouchBar

static NSTouchBarItemIdentifier HBTouchBarMain = @"fr.handbrake.previewWindowTouchBar";

static NSTouchBarItemIdentifier HBTouchBarRip = @"fr.handbrake.rip";
static NSTouchBarItemIdentifier HBTouchBarScrubber = @"fr.handbrake.scrubber";
static NSTouchBarItemIdentifier HBTouchBarFitToScreen = @"fr.handbrake.fitToScreen";

@dynamic touchBar;

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[HBTouchBarRip, NSTouchBarItemIdentifierFlexibleSpace, HBTouchBarScrubber, NSTouchBarItemIdentifierFlexibleSpace, HBTouchBarFitToScreen];

    bar.customizationIdentifier = HBTouchBarMain;
    bar.customizationAllowedItemIdentifiers = @[HBTouchBarRip, HBTouchBarScrubber, HBTouchBarFitToScreen, NSTouchBarItemIdentifierFlexibleSpace];

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
    else if ([identifier isEqualTo:HBTouchBarScrubber])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Previews", @"Touch bar");

        NSScrubber *scrubber = [[NSScrubber alloc] init];
        scrubber.delegate = self;
        scrubber.dataSource = self;

        [scrubber registerClass:[HBThumbnailItemView class] forItemIdentifier:thumbnailScrubberItemIdentifier];

        NSScrubberLayout *scrubberLayout = [[NSScrubberFlowLayout alloc] init];
        scrubber.scrubberLayout = scrubberLayout;
        scrubber.showsAdditionalContentIndicators = YES;
        scrubber.selectedIndex = 0;
        scrubber.selectionOverlayStyle = [NSScrubberSelectionStyle outlineOverlayStyle];
        scrubber.continuous = YES;
        scrubber.mode = NSScrubberModeFree;
        scrubber.itemAlignment = NSScrubberAlignmentCenter;

        // Set the layout constraints on this scrubber so that it's 400 pixels wide.
        NSDictionary *items = NSDictionaryOfVariableBindings(scrubber);
        NSArray *theConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:[scrubber(500)]" options:0 metrics:nil views:items];
        [NSLayoutConstraint activateConstraints:theConstraints];

        item.view = scrubber;
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

- (void)_touchBar_reloadScrubberData
{
    NSScrubber *scrubber = (NSScrubber *)[[self.touchBar itemForIdentifier:HBTouchBarScrubber] view];
    [scrubber reloadData];
    if (self.selectedIndex < scrubber.numberOfItems)
    {
        scrubber.animator.selectedIndex = self.selectedIndex;
    }
}

- (void)_touchBar_updateScrubberSelectedIndex:(NSUInteger)selectedIndex
{
    NSScrubber *scrubber = (NSScrubber *)[[self.touchBar itemForIdentifier:HBTouchBarScrubber] view];
    scrubber.animator.selectedIndex = selectedIndex;
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
