//
//  HBViewValidation.h
//  HandBrake
//
//  Created by Damiano Galassi on 07/08/14.
//
//

#import <Foundation/Foundation.h>

@protocol HBViewValidation <NSObject>

@property (nonatomic, readwrite, getter=isEnabled) BOOL enabled;

@end
