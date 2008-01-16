//
//  NSFileManager+Authentication.m
//  Sparkle
//
//  Created by Andy Matuschak on 3/9/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

@interface NSFileManager (SUAuthenticationAdditions)
- (BOOL)movePathWithAuthentication:(NSString *)src toPath:(NSString *)dst;
@end
