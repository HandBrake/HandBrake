//
//  SUAppcastItem.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface SUAppcastItem : NSObject {
	NSString *title;
	NSDate *date;
	NSString *description;
	
	NSURL *releaseNotesURL;
	
	NSString *DSASignature;
	NSString *MD5Sum;
	
	NSURL *fileURL;
	NSString *fileVersion;
	NSString *versionString;
}

// Initializes with data from a dictionary provided by the RSS class.
- initWithDictionary:(NSDictionary *)dict;

- (NSString *)title;
- (void)setTitle:(NSString *)aTitle;

- (NSDate *)date;
- (void)setDate:(NSDate *)aDate;

- (NSString *)description;
- (void)setDescription:(NSString *)aDescription;

- (NSURL *)releaseNotesURL;
- (void)setReleaseNotesURL:(NSURL *)aReleaseNotesURL;

- (NSString *)DSASignature;
- (void)setDSASignature:(NSString *)aDSASignature;

- (NSString *)MD5Sum;
- (void)setMD5Sum:(NSString *)aMd5Sum;

- (NSURL *)fileURL;
- (void)setFileURL:(NSURL *)aFileURL;

- (NSString *)fileVersion;
- (void)setFileVersion:(NSString *)aFileVersion;

- (NSString *)versionString;
- (void)setVersionString:(NSString *)versionString;

@end
