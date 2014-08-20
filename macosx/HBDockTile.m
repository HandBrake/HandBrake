//
//  HBDockTile.m
//  HandBrake
//
//  Created by Damiano Galassi on 20/08/14.
//
//

#import "HBDockTile.h"
#import "DockTextField.h"

NSString *dockTilePercentFormat = @"%2.1f%%";

@interface HBDockTile ()

@property (nonatomic, retain) NSDockTile *dockTile;
@property (nonatomic, retain) NSImage *image;
@property (nonatomic, retain) DockTextField * percentField;
@property (nonatomic, retain) DockTextField * timeField;

@end

@implementation HBDockTile

- (instancetype)initWithDockTile:(NSDockTile *)dockTile image:(NSImage *)image
{
    self = [super init];
    if (self)
    {
        _dockTile = [dockTile retain];
        _image = [image retain];

        NSImageView *iv = [[NSImageView alloc] init];
        [iv setImage:_image];
        [dockTile setContentView:iv];

        _percentField = [[DockTextField alloc] initWithFrame:NSMakeRect(0.0f, 32.0f, [dockTile size].width, 30.0f)];
        [_percentField changeGradientColors:[NSColor colorWithDeviceRed:0.4f green:0.6f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.2f green:0.4f blue:0.2f alpha:1.0f]];
        [iv addSubview:_percentField];

        _timeField = [[DockTextField alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, [dockTile size].width, 30.0f)];
        [_timeField changeGradientColors:[NSColor colorWithDeviceRed:0.6f green:0.4f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.4f green:0.2f blue:0.2f alpha:1.0f]];
        [iv addSubview:_timeField];
        [iv release];
    }
    return self;
}

- (void)dealloc
{
    [_dockTile release];
    [_image release];
    [_percentField release];
    [_timeField release];

    [super dealloc];
}

- (void)updateDockIcon:(double)progress withETA:(NSString *)etaStr
{
    if (progress < 0.0 || progress > 1.0)
    {
        [_percentField setHidden:YES];
        [_timeField setHidden:YES];
    }
    else
    {
        [_percentField setTextToDisplay:[NSString stringWithFormat:dockTilePercentFormat, progress * 100]];
        [_percentField setHidden:NO];
        [_timeField setTextToDisplay:etaStr];
        [_timeField setHidden:NO];
    }

    [_dockTile display];
}

@end
