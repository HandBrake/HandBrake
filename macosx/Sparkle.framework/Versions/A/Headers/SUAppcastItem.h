//
//  SUAppcastItem.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#ifndef SUAPPCASTITEM_H
#define SUAPPCASTITEM_H

#if __has_feature(modules)
@import Foundation;
#else
#import <Foundation/Foundation.h>
#endif
#import "SUExport.h"
@class SUSignatures;

SU_EXPORT @interface SUAppcastItem : NSObject
@property (copy, readonly) NSString *title;
@property (copy, readonly) NSString *dateString;
@property (copy, readonly) NSString *itemDescription;
@property (strong, readonly) NSURL *releaseNotesURL;
@property (strong, readonly) SUSignatures *signatures;
@property (copy, readonly) NSString *minimumSystemVersion;
@property (copy, readonly) NSString *maximumSystemVersion;
@property (strong, readonly) NSURL *fileURL;
@property (nonatomic, readonly) uint64_t contentLength;
@property (copy, readonly) NSString *versionString;
@property (copy, readonly) NSString *osString;
@property (copy, readonly) NSString *displayVersionString;
@property (copy, readonly) NSDictionary *deltaUpdates;
@property (strong, readonly) NSURL *infoURL;

// Initializes with data from a dictionary provided by the RSS class.
- (instancetype)initWithDictionary:(NSDictionary *)dict;
- (instancetype)initWithDictionary:(NSDictionary *)dict failureReason:(NSString **)error;

@property (getter=isDeltaUpdate, readonly) BOOL deltaUpdate;
@property (getter=isCriticalUpdate, readonly) BOOL criticalUpdate;
@property (getter=isMacOsUpdate, readonly) BOOL macOsUpdate;
@property (getter=isInformationOnlyUpdate, readonly) BOOL informationOnlyUpdate;

// Returns the dictionary provided in initWithDictionary; this might be useful later for extensions.
@property (readonly, copy) NSDictionary *propertiesDictionary;

- (NSURL *)infoURL;

@end

#endif
