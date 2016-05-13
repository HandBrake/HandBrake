/*  HBMutablePreset.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBMutablePreset.h"

@interface HBPreset (HBMutablePreset)

@property (nonatomic, strong, nullable) NSMutableDictionary *content;
- (void)cleanUp;

@end

@implementation HBMutablePreset

- (void)setObject:(id)obj forKey:(NSString *)key;
{
    self.content[key] = obj;
}

- (void)setObject:(id)obj forKeyedSubscript:(NSString *)key
{
    self.content[key] = obj;
}

- (void)cleanUp
{
    [super cleanUp];
}

@end
