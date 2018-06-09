/*  HBPictureHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPictureHUDController.h"

@interface HBPictureHUDController ()

@property (weak) IBOutlet NSTextField *scaleLabel;
@property (weak) IBOutlet NSTextField *infoLabel;

@property (weak) IBOutlet NSSlider *slider;

@property (weak) IBOutlet NSPopUpButton *durationPopUp;
@property (weak) IBOutlet NSButton *scaleToScreenButton;

@property (weak) IBOutlet NSTextField *durationLabel;
@property (weak) IBOutlet NSTextField *durationUnitLabel;

@property (nonatomic) BOOL fitToView;

@end

@implementation HBPictureHUDController

- (NSString *)nibName
{
    return @"HBPictureHUDController";
}

- (void)loadView
{
    [super loadView];

    if (NSClassFromString(@"NSVisualEffectView") == NO)
    {
        self.scaleLabel.textColor = [NSColor whiteColor];
        self.infoLabel.textColor = [NSColor whiteColor];
        self.durationLabel.textColor = [NSColor whiteColor];
        self.durationUnitLabel.textColor = [NSColor whiteColor];
    }

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

- (void)setPictureCount:(NSUInteger)pictureCount
{
    self.slider.numberOfTickMarks = pictureCount;
    self.slider.maxValue = pictureCount - 1;

    if (self.selectedIndex > pictureCount)
    {
        self.selectedIndex = pictureCount - 1;
    }
}

- (NSUInteger)selectedIndex
{
    return self.slider.integerValue;
}

- (void)setSelectedIndex:(NSUInteger)selectedIndex
{
    self.slider.integerValue = selectedIndex;
}

- (void)setInfo:(NSString *)info
{
    self.infoLabel.stringValue = info;
}

- (void)setScale:(NSString *)scale
{
    self.scaleLabel.stringValue = scale;
}

- (IBAction)previewDurationPopUpChanged:(id)sender
{
    [[NSUserDefaults standardUserDefaults] setObject:self.durationPopUp.titleOfSelectedItem forKey:@"PreviewLength"];
}

- (IBAction)pictureSliderChanged:(id)sender
{
    [self.delegate displayPreviewAtIndex:self.slider.integerValue];
}

- (IBAction)toggleScaleToScreen:(id)sender
{
    [self.delegate toggleScaleToScreen];
    if (self.fitToView == YES)
    {
        self.scaleToScreenButton.title = NSLocalizedString(@"Scale To Screen", @"Picture HUD -> scale button");
    }
    else
    {
        self.scaleToScreenButton.title = NSLocalizedString(@"Actual Scale", @"Picture HUD -> scale button");
    }
    self.fitToView = !self.fitToView;
}

- (IBAction)showPictureSettings:(id)sender
{
    [self.delegate showPictureSettings];
}

- (IBAction)createMoviePreview:(id)sender
{
    [self.delegate createMoviePreviewWithPictureIndex:self.slider.integerValue duration:self.durationPopUp.titleOfSelectedItem.intValue];
}

- (BOOL)HB_keyDown:(NSEvent *)event
{
    unichar key = [event.charactersIgnoringModifiers characterAtIndex:0];
    if (key == NSLeftArrowFunctionKey)
    {
        self.slider.integerValue = self.selectedIndex > self.slider.minValue ? self.selectedIndex - 1 : self.selectedIndex;
        [self.delegate displayPreviewAtIndex:self.slider.integerValue];
        return YES;
    }
    else if (key == NSRightArrowFunctionKey)
    {
        self.slider.integerValue = self.selectedIndex < self.slider.maxValue ? self.selectedIndex + 1 : self.selectedIndex;
        [self.delegate displayPreviewAtIndex:self.slider.integerValue];
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
        self.slider.integerValue = self.selectedIndex < self.slider.maxValue ? self.selectedIndex + 1 : self.selectedIndex;
        [self.delegate displayPreviewAtIndex:self.slider.integerValue];
    }
    else if (theEvent.deltaY > 0)
    {
        self.slider.integerValue = self.selectedIndex > self.slider.minValue ? self.selectedIndex - 1 : self.selectedIndex;
        [self.delegate displayPreviewAtIndex:self.slider.integerValue];
    }
    return YES;
}

@end
