//
//  SPUUpdatePermissionRequest.h
//  Sparkle
//
//  Created by Mayur Pawashe on 8/14/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Sparkle/SUExport.h>

NS_ASSUME_NONNULL_BEGIN

/*!
 This class represents information needed to make a permission request for checking updates.
 */
SU_EXPORT @interface SPUUpdatePermissionRequest : NSObject<NSSecureCoding>

/*!
 Initializes a new update permission request instance.
 
 @param systemProfile The system profile information.
 */
- (instancetype)initWithSystemProfile:(NSArray<NSDictionary<NSString *, NSString *> *> *)systemProfile;

/*!
 A read-only property for the user's system profile.
 */
@property (nonatomic, readonly) NSArray<NSDictionary<NSString *, NSString *> *> *systemProfile;

@end

NS_ASSUME_NONNULL_END
