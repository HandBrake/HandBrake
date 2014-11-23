/*  HBPreset.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBTreeNode.h"

/**
 *  HBPreset
 *  Stores a preset dictionary.
 *
 *  An instance of HBPreset can be an actual preset or a folder.
 */
@interface HBPreset : HBTreeNode <NSCopying>

- (instancetype)initWithName:(NSString *)title content:(NSDictionary *)content builtIn:(BOOL)builtIn;
- (instancetype)initWithFolderName:(NSString *)title builtIn:(BOOL)builtIn;

@property (nonatomic, copy) NSString *name;
@property (nonatomic, copy) NSString *presetDescription;
@property (nonatomic, retain) NSDictionary *content;

@property (nonatomic) BOOL isDefault;
@property (nonatomic, readonly) BOOL isBuiltIn;

@end
