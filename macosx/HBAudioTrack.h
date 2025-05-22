/*  HBAudioTrack.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBAudioTrack;
@class HBTitleAudioTrack;

NS_ASSUME_NONNULL_BEGIN

@protocol HBAudioTrackDataSource <NSObject>
- (HBTitleAudioTrack *)sourceTrackAtIndex:(NSUInteger)idx;
- (NSArray<NSString *> *)sourceTracksArray;
- (nullable NSString *)defaultTitleForTrackAtIndex:(NSUInteger)idx mixdown:(int)mixdown;
@end

@protocol HBAudioTrackDelegate <NSObject>
- (void)track:(HBAudioTrack *)track didChangeSourceFrom:(NSUInteger)oldSourceIdx;
- (void)encoderChanged;
@end

@interface HBAudioTrack : NSObject <NSSecureCoding, NSCopying>

- (instancetype)initWithTrackIdx:(NSUInteger)index
                       container:(int)container
                      dataSource:(id<HBAudioTrackDataSource>)dataSource
                        delegate:(id<HBAudioTrackDelegate>)delegate;

/// The index of the source in the data source tracks array.
@property (nonatomic, readwrite) NSUInteger sourceTrackIdx;
@property (nonatomic, readwrite) int container;

@property (nonatomic, weak, nullable) id<HBAudioTrackDataSource> dataSource;
@property (nonatomic, weak, nullable) id<HBAudioTrackDelegate> delegate;

/**
 *  track properties.
 */
@property (nonatomic, readwrite) int encoder;
@property (nonatomic, readwrite) int mixdown;
@property (nonatomic, readwrite) int sampleRate;
@property (nonatomic, readwrite) int bitRate;

@property (nonatomic, readwrite) double gain;
@property (nonatomic, readwrite) double drc;

@property (nonatomic, readwrite, nullable) NSString *title;

@property (nonatomic, readonly, getter=isEnabled) BOOL enabled;

/**
 *  Arrays of possible options for the track properties.
 */
@property (nonatomic, readonly) NSArray<NSString *> *encoders;
@property (nonatomic, readonly) NSArray<NSString *> *mixdowns;
@property (nonatomic, readonly) NSArray<NSString *> *sampleRates;
@property (nonatomic, readonly) NSArray<NSString *> *bitRates;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
