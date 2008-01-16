//
//  NSFileManager+Verification.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/16/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>

// For the paranoid folks!
@interface NSFileManager (SUVerification)
- (BOOL)validatePath:(NSString *)path withMD5Hash:(NSString *)hash;
- (BOOL)validatePath:(NSString *)path withEncodedDSASignature:(NSString *)encodedSignature;
@end
