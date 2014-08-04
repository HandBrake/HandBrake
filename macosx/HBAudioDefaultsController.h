/*  HBAudioDefaultsController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBAudioSettings;

@interface HBAudioDefaultsController : NSWindowController

- (instancetype)initWithSettings:(HBAudioSettings *)settings;

@property (nonatomic, readwrite, assign) id delegate;

@end
