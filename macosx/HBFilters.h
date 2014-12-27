/*  HBFilters.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

extern NSString * const HBFiltersChangedNotification;

/**
 *  Filters settings.
 */
@interface HBFilters : NSObject <NSCoding, NSCopying, HBPresetCoding>

@property (nonatomic, readwrite) NSInteger detelecine;
@property (nonatomic, readwrite, copy) NSString *detelecineCustomString;

@property (nonatomic, readwrite) BOOL useDecomb;

@property (nonatomic, readwrite) NSInteger deinterlace;
@property (nonatomic, readwrite, copy) NSString *deinterlaceCustomString;

@property (nonatomic, readwrite) NSInteger decomb;
@property (nonatomic, readwrite, copy) NSString *decombCustomString;

@property (nonatomic, readwrite, copy) NSString *denoise;
@property (nonatomic, readwrite, copy) NSString *denoisePreset;
@property (nonatomic, readwrite, copy) NSString *denoiseTune;
@property (nonatomic, readwrite, copy) NSString *denoiseCustomString;

@property (nonatomic, readwrite) NSInteger deblock;
@property (nonatomic, readwrite) BOOL grayscale;

@end
