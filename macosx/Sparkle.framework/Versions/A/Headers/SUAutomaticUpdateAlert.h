//
//  SUAutomaticUpdateAlert.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/18/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SUAppcastItem;
@interface SUAutomaticUpdateAlert : NSWindowController {
	SUAppcastItem *updateItem;
}

- initWithAppcastItem:(SUAppcastItem *)item;

- (IBAction)relaunchNow:sender;
- (IBAction)relaunchLater:sender;

@end
