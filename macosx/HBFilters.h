/*  HBFilters.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBFiltersChangedNotification;

/**
 *  Filters settings.
 */
@interface HBFilters : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

@property (nonatomic, readwrite, copy) NSString *detelecine;
@property (nonatomic, readwrite, copy) NSString *detelecineCustomString;

@property (nonatomic, readwrite) BOOL useDecomb;

@property (nonatomic, readwrite, copy) NSString *deinterlace;
@property (nonatomic, readwrite, copy) NSString *deinterlaceCustomString;

@property (nonatomic, readwrite, copy) NSString *decomb;
@property (nonatomic, readwrite, copy) NSString *decombCustomString;

@property (nonatomic, readwrite, copy) NSString *denoise;
@property (nonatomic, readwrite, copy) NSString *denoisePreset;
@property (nonatomic, readwrite, copy) NSString *denoiseTune;
@property (nonatomic, readwrite, copy) NSString *denoiseCustomString;

@property (nonatomic, readwrite) NSInteger deblock;
@property (nonatomic, readwrite) BOOL grayscale;

@end

NS_ASSUME_NONNULL_END
