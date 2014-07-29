/*  HBSubtitlesDefaultsController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBSubtitlesSettings;

@interface HBSubtitlesDefaultsController : NSWindowController

- (instancetype)initWithSettings:(HBSubtitlesSettings *)settings;

@property (nonatomic, readwrite, assign) id delegate;

@end
