/*  HBVideoController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

#include "hb.h"

@class HBController;
@class HBAdvancedController;

extern NSString *HBVideoEncoderChangedNotification;

/**
 *  HBVideoController
 *
 *  Responds to HBContainerChangedNotification and HBTitleChangedNotification notifications.
 */
@interface HBVideoController : NSViewController <HBViewValidation>

// Methods to apply the settings to the controller
- (void)applyVideoSettingsFromQueue:(NSDictionary *)queueToApply;
- (void)applySettingsFromPreset:(NSDictionary *)preset;

// Methods to get back the controller settings
- (void)prepareVideoForQueueFileJob:(NSMutableDictionary *)queueFileJob;
- (void)prepareVideoForJobPreview:(hb_job_t *)job andTitle:(hb_title_t *)title;
- (void)prepareVideoForPreset:(NSMutableDictionary *)preset;

- (IBAction)x264PresetsChangedDisplayExpandedOptions:(id)sender;

@property (nonatomic, copy, readwrite) NSString *pictureSettingsField;
@property (nonatomic, copy, readwrite) NSString *pictureFiltersField;

// Property exposed for the auto name function
@property (nonatomic, readonly) int codec;
@property (nonatomic, readonly) int qualityType;
@property (nonatomic, readonly) NSString *selectedBitrate;
@property (nonatomic, readonly) NSString *selectedQuality;

// Property updates when the video size changes
@property (nonatomic, readwrite) NSUInteger fPresetsWidthForUnparse;
@property (nonatomic, readwrite) NSUInteger fPresetsHeightForUnparse;

@property (nonatomic, retain, readwrite) HBController *fHBController;
@property (nonatomic, retain, readwrite) HBAdvancedController *fAdvancedOptions;

@end
