/*  HBDockTile.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBDockTile.h"
#import "HBDockTextField.h"

@interface HBDockTile ()

@property (nonatomic, strong) NSDockTile *dockTile;
@property (nonatomic, strong) NSImage *image;
@property (nonatomic, strong) HBDockTextField *percentField;
@property (nonatomic, strong) HBDockTextField *timeField;

@end

@implementation HBDockTile

- (instancetype)init
{
    @throw nil;
}

- (instancetype)initWithDockTile:(NSDockTile *)dockTile image:(NSImage *)image
{
    self = [super init];
    if (self)
    {
        _dockTile = dockTile;
        _image = image;

        NSImageView *iv = [[NSImageView alloc] init];
        iv.image = _image;
        dockTile.contentView = iv;

        _percentField = [[HBDockTextField alloc] initWithFrame:NSMakeRect(0.0f, 32.0f, dockTile.size.width, 30.0f)];
        _percentField.hidden = YES;
        [_percentField changeGradientColors:[NSColor colorWithDeviceRed:0.4f green:0.6f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.2f green:0.4f blue:0.2f alpha:1.0f]];
        [iv addSubview:_percentField];

        _timeField = [[HBDockTextField alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, dockTile.size.width, 30.0f)];
        _timeField.hidden = YES;
        [_timeField changeGradientColors:[NSColor colorWithDeviceRed:0.6f green:0.4f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.4f green:0.2f blue:0.2f alpha:1.0f]];
        [iv addSubview:_timeField];
    }
    return self;
}

- (void)setStringValue:(NSString *)stringValue
{
    if (stringValue.length)
    {
        _timeField.hidden = NO;
        _timeField.stringValue = stringValue;
    }
    else
    {
        _timeField.hidden = YES;
        _timeField.stringValue = @"";
    }

    _percentField.hidden = YES;
    [_dockTile display];
}

- (void)setProgress:(double)progress ETA:(NSString *)etaStr
{
    if (progress < 0.0 || progress > 1.0)
    {
        _percentField.hidden = YES;
        _timeField.hidden =YES;
    }
    else
    {
        _percentField.stringValue = [NSString stringWithFormat:@"%2.1f%%", progress * 100];
        _percentField.hidden = NO;
        _timeField.stringValue = etaStr;
        _timeField.hidden =NO;
    }

    [_dockTile display];
}

- (void)setProgress:(double)progress hours:(NSInteger)hours minutes:(NSInteger)minutes seconds:(NSInteger)seconds
{
    // ETA format is [XX]X:XX:XX when ETA is greater than one hour
    // [X]X:XX when ETA is greater than 0 (minutes or seconds)
    // When these conditions doesn't applied (eg. when ETA is undefined)
    // we show just a tilde (~)

    NSString *etaStr;
    if (hours > 0)
    {
        etaStr = [NSString stringWithFormat:@"%ld:%02ld:%02ld", (long)hours, (long)minutes, (long)seconds];
    }
    else if (minutes > 0 || seconds > 0)
    {
        etaStr = [NSString stringWithFormat:@"%ld:%02ld", (long)minutes, (long)seconds];
    }
    else
    {
        etaStr = @"~";
    }

    [self setProgress:progress ETA:etaStr];
}


@end
