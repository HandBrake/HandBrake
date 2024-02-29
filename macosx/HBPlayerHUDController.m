/*  HBPlayerHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPlayerHUDController.h"
#import "HBAttributedStringAdditions.h"

@interface HBPlayerHUDController ()

@property (nonatomic, weak) IBOutlet NSButton *playButton;
@property (nonatomic, weak) IBOutlet NSButton *beginButton;
@property (nonatomic, weak) IBOutlet NSButton *endButton;
@property (nonatomic, weak) IBOutlet NSSlider *slider;

@property (nonatomic, weak) IBOutlet NSButton *volumeButton;
@property (nonatomic, weak) IBOutlet NSSlider *volumeSlider;

@property (nonatomic, weak) IBOutlet NSTextField *currentTimeLabel;
@property (nonatomic, weak) IBOutlet NSTextField *remainingTimeLabel;

@property (nonatomic, weak) IBOutlet NSPopUpButton *tracksSelection;

@property (nonatomic, readwrite) id rateObserver;
@property (nonatomic, readwrite) id periodicObserver;

@end

@interface HBPlayerHUDController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
- (void)_touchBar_updatePlayState:(BOOL)playing;
- (void)_touchBar_updateMaxDuration:(NSTimeInterval)duration;
- (void)_touchBar_updateTime:(NSTimeInterval)currentTime duration:(NSTimeInterval)duration;
@end

@implementation HBPlayerHUDController

- (NSString *)nibName
{
    return @"HBPlayerHUDController";
}

- (BOOL)canBeHidden
{
    return YES;
}

- (void)setPlayer:(id<HBPlayer>)player
{
    if (_player)
    {
        [self.player removeRateObserver:self.rateObserver];
        [self.player removeTimeObserver:self.periodicObserver];
        self.rateObserver = nil;
        self.periodicObserver = nil;
        [self _clearTracksMenu];
    }

    _player = player;

    if (player)
    {
        [self _buildTracksMenu];

        __weak HBPlayerHUDController *weakSelf = self;

        self.periodicObserver = [self.player addPeriodicTimeObserverUsingBlock:^(NSTimeInterval time) {
            [weakSelf _refreshUI];
        }];

        self.rateObserver = [self.player addRateObserverUsingBlock:^{
            [weakSelf _refreshPlayButtonState];
        }];

        NSTimeInterval duration = self.player.duration;
        [self.slider setMinValue:0.0];
        [self.slider setMaxValue:duration];
        [self.slider setDoubleValue:0.0];

        [self _touchBar_updateMaxDuration:duration];

        self.player.volume = self.volumeSlider.floatValue;

        [self.player play];
    }
}

- (void)viewDidLoad
{
    if (@available(macOS 11, *))
    {
        self.playButton.imageScaling = NSImageScaleProportionallyUpOrDown;

        self.beginButton.imageScaling = NSImageScaleProportionallyUpOrDown;
        self.beginButton.image = [NSImage imageWithSystemSymbolName:@"backward.end.alt.fill" accessibilityDescription:nil];

        self.endButton.image = [NSImage imageWithSystemSymbolName:@"forward.end.alt.fill" accessibilityDescription:nil];
        self.endButton.imageScaling = NSImageScaleProportionallyUpOrDown;

        self.volumeButton.image = [NSImage imageWithSystemSymbolName:@"speaker.wave.3.fill" accessibilityDescription:nil];
    }
}

- (void)dealloc
{
    if (_rateObserver)
    {
        [_player removeRateObserver:_rateObserver];
        _rateObserver = nil;
    }
    if (_periodicObserver)
    {
        [_player removeTimeObserver:_periodicObserver];
        _periodicObserver = nil;
    }
}

#pragma mark - Audio and subtitles selection menu

- (void)_buildTracksMenu
{
    [self _clearTracksMenu];

    NSArray<HBPlayerTrack *> *audioTracks = self.player.audioTracks;
    if (audioTracks.count)
    {
        [self _addSectionTitle:NSLocalizedString(@"Audio", @"Player HUD -> audio menu")];
        [self _addTracksItemFromArray:audioTracks selector:@selector(enableAudioTrack:)];
    }

    NSArray<HBPlayerTrack *>  *subtitlesTracks = self.player.subtitlesTracks;
    if (subtitlesTracks.count)
    {
        if (audioTracks.count)
        {
            [self.tracksSelection.menu addItem:[NSMenuItem separatorItem]];
        }
        [self _addSectionTitle:NSLocalizedString(@"Subtitles", @"Player HUD -> subtitles menu")];
        [self _addTracksItemFromArray:subtitlesTracks selector:@selector(enableSubtitlesTrack:)];
    }
}

- (void)_clearTracksMenu
{
    for (NSMenuItem *item in [self.tracksSelection.menu.itemArray copy])
    {
        if (item.tag != 1)
        {
            [self.tracksSelection.menu removeItem:item];
        }
    }
}

- (void)_addSectionTitle:(NSString *)title
{
    NSMenuItem *sectionTitle = [[NSMenuItem alloc] init];
    sectionTitle.title = title;
    sectionTitle.enabled = NO;
    sectionTitle.indentationLevel = 0;
    [self.tracksSelection.menu addItem:sectionTitle];
}

- (void)_addTracksItemFromArray:(NSArray<HBPlayerTrack *> *)tracks selector:(SEL)selector
{
    for (HBPlayerTrack *track in tracks)
    {
        NSMenuItem *item = [[NSMenuItem alloc] init];
        item.title = track.name;
        item.enabled = YES;
        item.indentationLevel = 1;
        item.action = selector;
        item.target = self;
        item.state = track.enabled;
        item.representedObject = track;
        [self.tracksSelection.menu addItem:item];
    }
}

- (void)_updateTracksMenuState
{
    for (NSMenuItem *item in self.tracksSelection.menu.itemArray)
    {
        if (item.representedObject)
        {
            HBPlayerTrack *track = (HBPlayerTrack *)item.representedObject;
            item.state = track.enabled;
        }
    }
}

- (IBAction)enableAudioTrack:(NSMenuItem *)sender
{
    [self.player enableAudioTrack:sender.representedObject];
    [self _updateTracksMenuState];
}

- (IBAction)enableSubtitlesTrack:(NSMenuItem *)sender
{
    [self.player enableSubtitlesTrack:sender.representedObject];
    [self _updateTracksMenuState];
}

- (NSString *)_timeToTimecode:(NSTimeInterval)timeInSeconds
{
    UInt16 seconds = (UInt16)fmod(timeInSeconds, 60.0);
    UInt16 minutes = (UInt16)fmod(timeInSeconds / 60.0, 60.0);
    UInt16 milliseconds = (UInt16)((timeInSeconds - (int) timeInSeconds) * 1000);

    return [NSString stringWithFormat:@"%02d:%02d.%03d", minutes, seconds, milliseconds];
}

- (void)_refreshUI
{
    if (self.player)
    {
        NSTimeInterval currentTime = self.player.currentTime;
        NSTimeInterval duration = self.player.duration;

        self.slider.doubleValue = currentTime;
        self.currentTimeLabel.attributedStringValue = [self _timeToTimecode:currentTime].HB_smallMonospacedString;
        self.remainingTimeLabel.attributedStringValue = [self _timeToTimecode:duration - currentTime].HB_smallMonospacedString;

        [self _touchBar_updateTime:currentTime duration:duration];
    }
}

- (void)_refreshPlayButtonState
{
    BOOL playing = self.player.rate != 0.0;
    if (playing)
    {
        if (@available(macOS 11, *))
        {
            self.playButton.image = [NSImage imageWithSystemSymbolName:@"pause.fill" accessibilityDescription:nil];
        }
        else
        {
            self.playButton.image = [NSImage imageNamed:@"PauseTemplate"];
        }
    }
    else
    {
        if (@available(macOS 11, *))
        {
            self.playButton.image = [NSImage imageWithSystemSymbolName:@"play.fill" accessibilityDescription:nil];
        }
        else
        {
            self.playButton.image = [NSImage imageNamed:@"PlayTemplate"];
        }
    }

    [self _touchBar_updatePlayState:playing];
}

- (IBAction)playPauseToggle:(id)sender
{
    if (self.player.rate != 0.0)
    {
        [self.player pause];
    }
    else
    {
        [self.player play];
    }
}

- (IBAction)goToBeginning:(id)sender
{
    [self.player gotoBeginning];
}

- (IBAction)goToEnd:(id)sender
{
    [self.player gotoEnd];
}

- (IBAction)showPicturesPreview:(id)sender
{
    [self.delegate stopPlayer];
}

- (IBAction)sliderChanged:(NSSlider *)sender
{
    self.player.currentTime = sender.doubleValue;
}

- (IBAction)maxVolume:(id)sender
{
    self.volumeSlider.doubleValue = 1;
    self.player.volume = 1;
}

- (IBAction)mute:(id)sender
{
    if (self.volumeSlider.doubleValue == 0)
    {
        self.volumeSlider.doubleValue = 1;
        self.player.volume = 1;
    }
    else
    {
        self.volumeSlider.doubleValue = 0;
        self.player.volume = 0;
    }
    [self volumeSliderChanged:self.volumeSlider];
}

- (IBAction)volumeSliderChanged:(NSSlider *)sender
{
    float value = sender.floatValue;
    self.player.volume = value;

    if (@available(macOS 11, *))
    {
        if (value == 0)
        {
            self.volumeButton.image = [NSImage imageWithSystemSymbolName:@"speaker.slash.fill" accessibilityDescription:nil];
        }
        else if (value > 0 && value <= 0.33)
        {
            self.volumeButton.image = [NSImage imageWithSystemSymbolName:@"speaker.wave.1.fill" accessibilityDescription:nil];
        }
        else if (value > 0.33 && value <= 0.66)
        {
            self.volumeButton.image = [NSImage imageWithSystemSymbolName:@"speaker.wave.2.fill" accessibilityDescription:nil];
        }
        else if (value > 0.66)
        {
            self.volumeButton.image = [NSImage imageWithSystemSymbolName:@"speaker.wave.3.fill" accessibilityDescription:nil];
        }
    }
    else
    {
        if (value == 0)
        {
            self.volumeButton.image = [NSImage imageNamed:@"volLowTemplate"];
        }
        else
        {
            self.volumeButton.image = [NSImage imageNamed:@"volHighTemplate"];
        }
    }
}

#pragma mark - Keyboard and mouse wheel control

- (BOOL)HB_keyDown:(NSEvent *)event
{
    unichar key = [event.charactersIgnoringModifiers characterAtIndex:0];

    if (self.player)
    {
        if (key == 32)
        {
            if (self.player.rate != 0.0)
                [self.player pause];
            else
                [self.player play];
        }
        else if (key == 'k')
            [self.player pause];
        else if (key == 'l')
        {
            float rate = self.player.rate;
            rate += 1.0f;
            [self.player play];
            self.player.rate = rate;
        }
        else if (key == 'j')
        {
            float rate = self.player.rate;
            rate -= 1.0f;
            [self.player play];
            self.player.rate = rate;
        }
        else if (event.modifierFlags & NSEventModifierFlagOption && key == NSLeftArrowFunctionKey)
        {
            [self.player gotoBeginning];
        }
        else if (event.modifierFlags & NSEventModifierFlagOption && key == NSRightArrowFunctionKey)
        {
            [self.player gotoEnd];
        }
        else if (key == NSLeftArrowFunctionKey)
        {
            [self.player stepBackward];
        }
        else if (key == NSRightArrowFunctionKey)
        {
            [self.player stepForward];
        }
        else
        {
            return NO;
        }
    }
    else
    {
        return NO;
    }

    return YES;
}

- (BOOL)HB_scrollWheel:(NSEvent *)theEvent
{
    if (theEvent.deltaY < 0)
    {
        self.player.currentTime = self.player.currentTime + 0.5;
    }
    else if (theEvent.deltaY > 0)
    {
        self.player.currentTime = self.player.currentTime - 0.5;
    }
    return YES;
}

@end

@implementation HBPlayerHUDController (TouchBar)

static NSTouchBarItemIdentifier HBTouchBar = @"fr.handbrake.playerHUDTouchBar";

static NSTouchBarItemIdentifier HBTouchBarDone = @"fr.handbrake.done";
static NSTouchBarItemIdentifier HBTouchBarPlayPause = @"fr.handbrake.playPause";
static NSTouchBarItemIdentifier HBTouchBarCurrentTime = @"fr.handbrake.currentTime";
static NSTouchBarItemIdentifier HBTouchBarRemainingTime = @"fr.handbrake.remainingTime";
static NSTouchBarItemIdentifier HBTouchBarTimeSlider = @"fr.handbrake.timeSlider";

@dynamic touchBar;

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.escapeKeyReplacementItemIdentifier = HBTouchBarDone;

    bar.defaultItemIdentifiers = @[HBTouchBarPlayPause, NSTouchBarItemIdentifierFixedSpaceSmall, HBTouchBarCurrentTime, NSTouchBarItemIdentifierFixedSpaceSmall, HBTouchBarTimeSlider, NSTouchBarItemIdentifierFixedSpaceSmall, HBTouchBarRemainingTime];

    bar.customizationIdentifier = HBTouchBar;
    bar.customizationAllowedItemIdentifiers = @[HBTouchBarPlayPause, HBTouchBarCurrentTime, HBTouchBarTimeSlider, HBTouchBarRemainingTime, NSTouchBarItemIdentifierFlexibleSpace];

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarDone])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Done", @"Touch bar");

        NSButton *button = [NSButton buttonWithTitle:NSLocalizedString(@"Done", @"Touch bar") target:self action:@selector(showPicturesPreview:)];
        button.bezelColor = NSColor.systemYellowColor;

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarPlayPause])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Play/Pause", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarPlayTemplate]
                                              target:self action:@selector(playPauseToggle:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarCurrentTime])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Current Time", @"Touch bar");

        NSTextField *label = [NSTextField labelWithString:NSLocalizedString(@"--:--", @"")];

        item.view = label;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarRemainingTime])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Remaining Time", @"Touch bar");

        NSTextField *label = [NSTextField labelWithString:NSLocalizedString(@"- --:--", @"")];

        item.view = label;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarTimeSlider])
    {
        NSSliderTouchBarItem *item = [[NSSliderTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Slider", @"Touch bar");

        item.slider.minValue = 0.0f;
        item.slider.maxValue = 100.0f;
        item.slider.doubleValue = 0.0f;
        item.slider.continuous = YES;
        item.target = self;
        item.action = @selector(touchBarSliderChanged:);

        return item;
    }
    return nil;
}

- (void)touchBarSliderChanged:(NSSliderTouchBarItem *)sender
{
    [self sliderChanged:sender.slider];
}

- (void)_touchBar_updatePlayState:(BOOL)playing
{
    NSButton *playButton = (NSButton *)[[self.touchBar itemForIdentifier:HBTouchBarPlayPause] view];

    if (playing)
    {
        playButton.image = [NSImage imageNamed:NSImageNameTouchBarPauseTemplate];
    }
    else
    {
        playButton.image = [NSImage imageNamed:NSImageNameTouchBarPlayTemplate];
    }
}

- (void)_touchBar_updateMaxDuration:(NSTimeInterval)duration
{
    NSSlider *slider = (NSSlider *)[[self.touchBar itemForIdentifier:HBTouchBarTimeSlider] slider];
    slider.maxValue = duration;
}

- (NSString *)_timeToString:(NSTimeInterval)timeInSeconds negative:(BOOL)negative
{
    UInt16 seconds = (UInt16)fmod(timeInSeconds, 60.0);
    UInt16 minutes = (UInt16)fmod(timeInSeconds / 60.0, 60.0);

    if (negative)
    {
        return [NSString stringWithFormat:@"-%02d:%02d", minutes, seconds];
    }
    else
    {
        return [NSString stringWithFormat:@"%02d:%02d", minutes, seconds];
    }
}

- (void)_touchBar_updateTime:(NSTimeInterval)currentTime duration:(NSTimeInterval)duration
{
    NSSlider *slider = (NSSlider *)[[self.touchBar itemForIdentifier:HBTouchBarTimeSlider] slider];
    NSTextField *currentTimeLabel = (NSTextField *)[[self.touchBar itemForIdentifier:HBTouchBarCurrentTime] view];
    NSTextField *remainingTimeLabel = (NSTextField *)[[self.touchBar itemForIdentifier:HBTouchBarRemainingTime] view];

    slider.doubleValue = currentTime;
    currentTimeLabel.attributedStringValue = [self _timeToString:currentTime negative:NO].HB_monospacedString;
    remainingTimeLabel.attributedStringValue = [self _timeToString:duration - currentTime negative:YES].HB_monospacedString;
}

@end
