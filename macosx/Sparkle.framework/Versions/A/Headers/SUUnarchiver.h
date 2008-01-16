//
//  SUUnarchiver.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/16/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface SUUnarchiver : NSObject {
	id delegate;
}

- (void)unarchivePath:(NSString *)path;
- (void)setDelegate:delegate;

@end

@interface NSObject (SUUnarchiverDelegate)
- (void)unarchiver:(SUUnarchiver *)unarchiver extractedLength:(long)length;
- (void)unarchiverDidFinish:(SUUnarchiver *)unarchiver;
- (void)unarchiverDidFail:(SUUnarchiver *)unarchiver;
@end