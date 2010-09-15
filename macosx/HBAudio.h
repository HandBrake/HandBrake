//
//  HBAudio.h
//  HandBrake
//
//  Created on 2010-08-30.
//

#import <Cocoa/Cocoa.h>

@class HBAudioController;

extern NSString *keyAudioCodecName;
extern NSString *keyAudioSampleRateName;
extern NSString *keyAudioBitrateName;
extern NSString *keyAudioMixdownName;
extern NSString *keyAudioCodec;
extern NSString *keyAudioMixdown;
extern NSString *keyAudioSamplerate;
extern NSString *keyAudioBitrate;

@interface HBAudio : NSObject

{
	NSDictionary *track;
	NSDictionary *codec;
	NSDictionary *mixdown;
	NSDictionary *sampleRate;
	NSDictionary *bitRate;
	NSNumber *drc;
	NSNumber *videoContainerTag;
	HBAudioController *controller;
	NSMutableArray *codecs;
	NSMutableArray *mixdowns;
	NSMutableArray *bitRates;
}

@property (nonatomic, retain) NSDictionary *track;
@property (nonatomic, retain) NSDictionary *codec;
@property (nonatomic, retain) NSDictionary *mixdown;
@property (nonatomic, retain) NSDictionary *sampleRate;
@property (nonatomic, retain) NSDictionary *bitRate;
@property (nonatomic, retain) NSNumber *drc;
@property (nonatomic, retain) NSNumber *videoContainerTag;
@property (nonatomic, assign) HBAudioController *controller;

@property (nonatomic, readonly) NSArray *tracks;
@property (nonatomic, retain) NSMutableArray *codecs;
@property (nonatomic, retain) NSMutableArray *mixdowns;
@property (nonatomic, readonly) NSArray *sampleRates;
@property (nonatomic, retain) NSArray *bitRates;
@property (nonatomic, readonly) BOOL enabled;

- (void) setTrackFromIndex: (int) aValue;
- (BOOL) setCodecFromName: (NSString *) aValue;
- (void) setMixdownFromName: (NSString *) aValue;
- (void) setSampleRateFromName: (NSString *) aValue;
- (void) setBitRateFromName: (NSString *) aValue;

@end

