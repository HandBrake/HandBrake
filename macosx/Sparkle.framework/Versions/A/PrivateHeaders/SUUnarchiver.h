//
//  SUUnarchiver.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/16/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SUUnarchiverProtocol;

@interface SUUnarchiver : NSObject

+ (nullable id <SUUnarchiverProtocol>)unarchiverForPath:(NSString *)path updatingHostBundlePath:(nullable NSString *)hostPath decryptionPassword:(nullable NSString *)decryptionPassword;

@end

NS_ASSUME_NONNULL_END
