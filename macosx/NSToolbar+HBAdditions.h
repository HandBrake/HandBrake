/*  NSToolbar+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSUInteger, HBToolbarItemStyle) {
    HBToolbarItemStyleDefault  = 1 << 0,
    HBToolbarItemStyleBordered = 1 << 1,
    HBToolbarItemStyleButton   = 1 << 2,
};

@interface NSToolbar (HBAdditions)

- (nullable NSToolbarItem *)HB_toolbarItemWithIdentifier:(NSString *)itemIdentifier;
- (nullable NSToolbarItem *)HB_visibleToolbarItemWithIdentifier:(NSString *)itemIdentifier;

@end

@interface NSToolbarItem (HBAdditions)

+ (instancetype)HB_toolbarItemWithIdentifier:(NSString *)itemIdentifier
                                       label:(NSString *)label
                                paletteLabel:(nullable NSString *)palettelabel
                                  symbolName:(NSString *)symbolName
                                       image:(NSString *)imageName
                                       style:(HBToolbarItemStyle)style
                                      action:(SEL)action;

- (void)HB_setSymbol:(NSString *)symbolName configuration:(nullable id)configuration fallbackImage:(NSString *)imageName;

@end

NS_ASSUME_NONNULL_END
