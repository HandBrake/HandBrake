//
//  HBPresetsViewController.h
//  PresetsView
//
//  Created by Damiano Galassi on 14/07/14.
//  Copyright (c) 2014 Damiano Galassi. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

@class HBPresetsManager;
@class HBPreset;

@protocol HBPresetsViewControllerDelegate <NSObject>

- (void)selectionDidChange;
- (void)showAddPresetPanel:(id)sender;

@end

@interface HBPresetsViewController : NSViewController <HBViewValidation>

- (instancetype)initWithPresetManager:(HBPresetsManager *)presetManager;

@property (nonatomic, readwrite, assign) id<HBPresetsViewControllerDelegate> delegate;

- (void)deselect;
- (void)selectDefaultPreset;

@property (nonatomic, readonly) HBPreset *selectedPreset;
@property (nonatomic, readonly) NSUInteger indexOfSelectedItem;

@end
