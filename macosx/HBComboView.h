/*  HBComboView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBComboView : NSView

- (instancetype)initWithTitle:(NSString *)title image:(NSImage *)image target:(id)target action:(SEL)action menu:(NSMenu *)menu;

@property (strong) NSMenu *menu;

@property (nonatomic) BOOL enabled;
@property (nonatomic) SEL action;

@end

NS_ASSUME_NONNULL_END
