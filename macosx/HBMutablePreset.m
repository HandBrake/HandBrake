//
//  HBMutablePreset.m
//  HandBrake
//
//  Created by Damiano Galassi on 12/10/15.
//
//

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
