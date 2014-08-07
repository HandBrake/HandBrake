//
//  HBBaseNode.h
//  PresetsView
//
//  Created by Damiano Galassi on 14/07/14.
//  Copyright (c) 2014 Damiano Galassi. All rights reserved.
//

#import <Cocoa/Cocoa.h>

/**
 *  HBPreset
 *  Stores a preset dictionary
 *  and implements the requited methods to work with a NSTreeController.
 */
@interface HBPreset : NSObject <NSCopying>

- (instancetype)initWithName:(NSString *)title content:(NSDictionary *)content builtIn:(BOOL)builtIn;
- (instancetype)initWithFolderName:(NSString *)title builtIn:(BOOL)builtIn;

@property (nonatomic, copy) NSString *name;
@property (nonatomic, readwrite, retain) NSString *presetDescription;
@property (nonatomic, retain) NSDictionary *content;

@property (nonatomic, readwrite) BOOL isDefault;
@property (nonatomic, readonly) BOOL isBuiltIn;

// NSTreeController required properties
@property (nonatomic, retain) NSMutableArray *children;
@property (nonatomic) BOOL isLeaf;

/**
 *  Executes a given block using each object in the tree, starting with the root object and continuing through the tree to the last object.
 *
 *  @param block The block to apply to elements in the tree.
 */
- (void)enumerateObjectsUsingBlock:(void (^)(id obj, NSIndexPath *idx, BOOL *stop))block;

@end
