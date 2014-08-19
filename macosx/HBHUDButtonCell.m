//
//  HBHUDButtonCell.m
//  HandBrake
//
//  Created by Damiano Galassi on 17/08/14.
//
//

#import "HBHUDButtonCell.h"

@implementation HBHUDButtonCell

- (NSRect)drawTitle:(NSAttributedString *)title withFrame:(NSRect)frame inView:(NSView *)controlView
{
    NSAttributedString *attrLabel = [[[NSAttributedString alloc] initWithString:[title string]
                                                                    attributes:@{ NSFontAttributeName:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:self.controlSize]],
                                                                                  NSForegroundColorAttributeName: [NSColor whiteColor]}] autorelease];

    return [super drawTitle:attrLabel withFrame:frame inView:controlView];
}

@end
