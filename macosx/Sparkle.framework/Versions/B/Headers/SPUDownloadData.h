//
//  SPUDownloadData.h
//  Sparkle
//
//  Created by Mayur Pawashe on 8/10/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 * A class for containing downloaded data along with some information about it.
 */
SU_EXPORT NS_SWIFT_SENDABLE @interface SPUDownloadData : NSObject <NSSecureCoding>

/**
 * The raw data that was downloaded.
 */
@property (nonatomic, readonly) NSData *data;

/**
 * The URL that was fetched from.
 *
 * This may be different from the URL in the request if there were redirects involved.
 */
@property (nonatomic, readonly, copy) NSURL *URL;

/**
 * The IANA charset encoding name if available. Eg: "utf-8"
 */
@property (nonatomic, readonly, nullable, copy) NSString *textEncodingName;

/**
 * The MIME type if available. Eg: "text/plain"
 */
@property (nonatomic, readonly, nullable, copy) NSString *MIMEType;

@end

NS_ASSUME_NONNULL_END
