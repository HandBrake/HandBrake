//
//  HBVisualEffectBox.m
//  HandBrake
//
//  Created by Toby on 21/08/14.
//
//

#import "HBHUDView.h"

@interface NSView (HBHUDViewExtension)

- (void)setBlendingMode:(int)mode;
- (void)setMaterial:(int)material;
- (void)setState:(int)state;

@end

@implementation HBHUDView

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        [self setupOldStyleHUD];
    }

    return self;
}

+ (void)setupNewStyleHUD:(NSView *)view
{
    [view setWantsLayer:YES];
    [view.layer setCornerRadius:4];

    // Hardcode the values so we can
    // compile it with the 10.9 sdk.
    [view setBlendingMode:1];
    [view setMaterial:2];
    [view setState:1];

    [view setAppearance:[NSAppearance appearanceNamed:@"NSAppearanceNameVibrantDark"]];
}

- (void)setupOldStyleHUD
{
    [self setWantsLayer:YES];
    [self.layer setCornerRadius:14];

    // Black transparent background and white border
    CGColorRef white = CGColorCreateGenericRGB(1.0, 1.0, 1.0, 0.9);
    [self.layer setBorderColor:white];
    CFRelease(white);
    [self.layer setBorderWidth:2];

    CGColorRef black = CGColorCreateGenericRGB(0.0, 0.0, 0.0, 0.6);
    [self.layer setBackgroundColor:black];
    CFRelease(black);
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if (NSClassFromString(@"NSVisualEffectView"))
    {
        // Return a NSVisualEffectView instance
        self = [[NSClassFromString(@"NSVisualEffectView") alloc] initWithFrame:frame];
        if (self)
        {
            [HBHUDView setupNewStyleHUD:self];
        }
    }
    else
    {
        self = [super initWithFrame:frame];
        if (self)
        {
            [self setupOldStyleHUD];
        }
    }

    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self)
    {
        [self setupOldStyleHUD];
    }
    return self;
}

@end