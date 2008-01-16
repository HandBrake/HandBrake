//
//  SUAppcast.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class RSS, SUAppcastItem;
@interface SUAppcast : NSObject {
	NSArray *items;
	id delegate;
}

- (void)fetchAppcastFromURL:(NSURL *)url;
- (void)setDelegate:delegate;

- (SUAppcastItem *)newestItem;
- (NSArray *)items;

@end

@interface NSObject (SUAppcastDelegate)
- appcastDidFinishLoading:(SUAppcast *)appcast;
@end