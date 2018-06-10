/*  HBPlayerHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPlayerHUDController.h"

@interface HBPlayerHUDController ()

@property (weak) IBOutlet NSButton *playButton;
@property (weak) IBOutlet NSSlider *slider;

@property (weak) IBOutlet NSSlider *volumeSlider;

@property (weak) IBOutlet NSTextField *currentTimeLabel;
@property (weak) IBOutlet NSTextField *remaingTimeLabel;

@property (weak) IBOutlet NSPopUpButton *tracksSelection;

@property (nonatomic, readonly) NSDictionary *monospacedAttr;

@property (nonatomic, readwrite) id rateObserver;
@property (nonatomic, readwrite) id periodicObserver;

@end

@implementation HBPlayerHUDController

- (NSString *)nibName
{
    return @"HBPlayerHUDController";
}

- (void)loadView
{
    [super loadView];

    if ([[NSFont class] respondsToSelector:@selector(monospacedDigitSystemFontOfSize:weight:)]) {
        _monospacedAttr = @{NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:[NSFont smallSystemFontSize] weight:NSFontWeightRegular]};
    }
    else {
        _monospacedAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:[NSFont smallSystemFontSize]]};
    }
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

        // 10.7 does not supports weak NSViewController,
        // so use self and disable the warning for now.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-retain-cycles"

        self.periodicObserver = [self.player addPeriodicTimeObserverUsingBlock:^(NSTimeInterval time) {
            [self _refreshUI];
        }];

        self.rateObserver = [self.player addRateObserverUsingBlock:^{
            if (self.player.rate != 0.0)
            {
                self.playButton.image = [NSImage imageNamed:@"PauseTemplate"];
            }
            else
            {
                self.playButton.image = [NSImage imageNamed:@"PlayTemplate"];
            }
        }];

        [self.slider setMinValue:0.0];
        [self.slider setMaxValue:self.player.duration];
        [self.slider setDoubleValue:0.0];

        self.player.volume = self.volumeSlider.floatValue;

        [self.player play];
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

- (NSAttributedString *)_monospacedString:(NSString *)string
{
    return [[NSAttributedString alloc] initWithString:string attributes:self.monospacedAttr];

}

- (void)_refreshUI
{
    if (self.player)
    {
        NSTimeInterval currentTime = self.player.currentTime;
        NSTimeInterval duration = self.player.duration;

        self.slider.doubleValue = currentTime;
        self.currentTimeLabel.attributedStringValue = [self _monospacedString:[self _timeToTimecode:currentTime]];
        self.remaingTimeLabel.attributedStringValue = [self _monospacedString:[self _timeToTimecode:duration - currentTime]];
    }
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
    self.volumeSlider.doubleValue = 0;
    self.player.volume = 0;
}

- (IBAction)volumeSliderChanged:(NSSlider *)sender
{
    self.player.volume = sender.floatValue;
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

- (BOOL)HB_scrollWheel:(NSEvent *)theEvent;
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
