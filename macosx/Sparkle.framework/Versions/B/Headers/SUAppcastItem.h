//
//  SUAppcastItem.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#ifndef SUAPPCASTITEM_H
#define SUAPPCASTITEM_H

#import <Foundation/Foundation.h>

#ifdef BUILDING_SPARKLE_SOURCES_EXTERNALLY
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#import "SPUAppcastSigningValidationStatus.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#import <Sparkle/SPUAppcastSigningValidationStatus.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 The appcast item describing an update in the application's appcast feed.

 An appcast item represents a single update item in the `SUAppcast`  contained within the @c <item> element.
 
 Every appcast item must have a `versionString`, and either a `fileURL` or an `infoURL`.
 All the remaining properties describing an update to the application are optional.
 
 Extended documentation and examples on using appcast item features are available at:
 https://sparkle-project.org/documentation/publishing/
 */
SU_EXPORT NS_SWIFT_SENDABLE @interface SUAppcastItem : NSObject<NSSecureCoding>

/**
 The version of the update item.
 
 Sparkle uses this property to compare update items and determine the best available update item in the `SUAppcast`.
 
 This corresponds to the application update's @c CFBundleVersion
 
 This is extracted from the @c <sparkle:version> element, or the @c sparkle:version attribute from the @c <enclosure> element.
 */
@property (nonatomic, copy, readonly) NSString *versionString;

/**
 The human-readable display version of the update item if provided.
 
 This is the version string shown to the user when they are notified of a new update.
 
 This corresponds to the application update's @c CFBundleShortVersionString
 
 This is extracted from the @c <sparkle:shortVersionString> element,  or the @c sparkle:shortVersionString attribute from the @c <enclosure> element.
 
 If no short version string is available, this falls back to the update's `versionString`.
 */
@property (nonatomic, copy, readonly) NSString *displayVersionString;

/**
 The file URL to the update item if provided.
 
 This download contains the actual update Sparkle will attempt to install.
 In cases where a download cannot be provided, an `infoURL` must be provided instead.
 
 A file URL should have an accompanying `contentLength` provided.
 
 This is extracted from the @c url attribute in the @c <enclosure> element.
 */
@property (nonatomic, readonly, nullable) NSURL *fileURL;

/**
 The content length of the download in bytes.
 
 This property is used as a fallback when the server doesn't report the content length of the download.
 In that case, it is used to report progress of the downloading update to the user.
 
 A warning is outputted if this property is not equal the server's expected content length (if provided).
 
 This is extracted from the @c length attribute in the @c <enclosure> element.
 It should be specified if a `fileURL` is provided.
 */
@property (nonatomic, readonly) uint64_t contentLength;

/**
 The info URL to the update item if provided.
 
 This informational link is used to direct the user to learn more about an update they cannot download/install directly from within the application.
 The link should point to the product's web page.
 
 The informational link will be used if `informationOnlyUpdate` is @c YES
 
 This is extracted from the @c <link> element.
 */
@property (nonatomic, readonly, nullable) NSURL *infoURL;

/**
 Indicates whether or not the update item is only informational and has no download.
 
 If `infoURL` is not present, this is @c NO
 
 If `fileURL` is not present, this is @c YES
 
 Otherwise this is determined based on the contents extracted from the @c <sparkle:informationalUpdate> element.
 */
@property (nonatomic, getter=isInformationOnlyUpdate, readonly) BOOL informationOnlyUpdate;

/**
 The title of the appcast item if provided.
 
 This is extracted from the @c <title> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *title;

/**
 The date string of the appcast item if provided.
 
 The `date` property is constructed from this property and expects this string to comply with the following date format:
 `E, dd MMM yyyy HH:mm:ss Z`
 
 This is extracted from the @c <pubDate> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *dateString;

/**
 The date constructed from the `dateString` property if provided.
 
 Sparkle by itself only uses this property for phased group rollouts specified via `phasedRolloutInterval`, but clients may query this property too.
 
 This date is constructed using the  @c en_US locale.
 */
@property (nonatomic, copy, readonly, nullable) NSDate *date;

/**
 The release notes URL of the appcast item if provided.
 
 This external link points to an HTML file that Sparkle downloads and renders to show the user a new or old update item's changelog.
 
 An alternative to using an external release notes link is providing an embedded `itemDescription`.
 
 This is extracted from the @c <sparkle:releaseNotesLink> element.
 */
@property (nonatomic, readonly, nullable) NSURL *releaseNotesURL;

/**
 The description of the appcast item if provided.
 
 A description may be provided for inline/embedded release notes for new updates using @c <![CDATA[...]]>
 This is an alternative to providing a `releaseNotesURL`.
 
 This is extracted from the @c <description> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *itemDescription;

/**
 The format of the `itemDescription` for inline/embedded release notes if provided.
 
 This may be:
 - @c html
 - @c plain-text
 
 This is extracted from the @c sparkle:descriptionFormat attribute in the @c <description> element.
 
 If the format is not provided in the @c <description> element of the appcast item, then this property may default to `html`.
 
 If the @c <description> element of the appcast item is not available, this property is `nil`.
 */
@property (nonatomic, readonly, nullable) NSString *itemDescriptionFormat;

/**
 The full release notes URL of the appcast item if provided.
 
 The link should point to the product's full changelog.
 
 Sparkle's standard user interface offers to show these full release notes when a user checks for a new update and no new update is available.
 
 This is extracted from the @c <sparkle:fullReleaseNotesLink> element.
 */
@property (nonatomic, readonly, nullable) NSURL *fullReleaseNotesURL;

/**
 The required minimum system operating version string for this update if provided.
 
 This version string should contain three period-separated components.
 
 Example: @c 12.0.0
 
 Use `minimumOperatingSystemVersionIsOK` property to test if the current running system passes this requirement.
 
 This is extracted from the @c <sparkle:minimumSystemVersion> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *minimumSystemVersion;

/**
 Indicates whether or not the current running system passes the `minimumSystemVersion` requirement.
 */
@property (nonatomic, readonly) BOOL minimumOperatingSystemVersionIsOK;

/**
 This update will be ignored if the application's version precedes this update's minimum version.
 
 This application's version corresponds to the bundle's @c CFBundleVersion
 
 The minimum update version is extracted from the @c <sparkle:minimumUpdateVersion> element.
 
 Use `minimumUpdateVersionIsOK` property to test if the current bundle passes this requirement.
 
 Old applications must be using Sparkle 2.9 or later, otherwise this property will be ignored.
 */
@property (nonatomic, copy, readonly, nullable) NSString *minimumUpdateVersion;

/**
 Indicates whether or not the current bundle passes the `minimumUpdateVersion` requirement.
 */
@property (nonatomic, readonly) BOOL minimumUpdateVersionIsOK;

/**
 The required maximum system operating version string for this update if provided.
 
 A maximum system operating version requirement should only be made in unusual scenarios.
 
 This version string should contain three period-separated components.
 
 Example: @c 10.14.0
 
 Use `maximumOperatingSystemVersionIsOK` property  to test if the current running system passes this requirement.
 
 This is extracted from the @c <sparkle:maximumSystemVersion> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *maximumSystemVersion;

/**
 Indicates whether or not the current running system passes the `maximumSystemVersion` requirement.
 */
@property (nonatomic, readonly) BOOL maximumOperatingSystemVersionIsOK;

/**
 The required hardware requirements for this update if provided.
 
 Example: @c arm64
 
 Use `arm64HardwareRequirementIsOK` property to test if the current running system passes the @c arm64 requirement.
 
 This is extracted from the @c <sparkle:hardwareRequirements> element, which is a comma-delimited list of hardware requirements.
 */
@property (nonatomic, copy, readonly) NSSet<NSString *> *hardwareRequirements;

/**
 Indicates whether or not the current running system passes the arm64 requirement if specified in the `hardwareRequirements` requirement.
 */
@property (nonatomic, readonly) BOOL arm64HardwareRequirementIsOK;

/**
 The channel the update item is on if provided.
 
 An update item may specify a custom channel name (such as @c beta) that can only be found by updaters that filter for that channel.
 If no channel is provided, the update item is assumed to be on the default channel.
 
 This is extracted from the @c <sparkle:channel> element.
 Old applications must be using Sparkle 2 or later to interpret the channel element and to ignore unmatched channels.
 */
@property (nonatomic, readonly, nullable) NSString *channel;

/**
 The installation type of the update at `fileURL`
 
 This may be:
 - @c application - indicates this is a regular application update.
 - @c package - indicates this is a package installer update.
 
 This is extracted from the @c sparkle:installationType attribute in the @c <enclosure> element.
 
 If no installation type is provided in the enclosure, the installation type is inferred from the `fileURL` file extension instead.
 
 If the file extension is @c pkg or @c mpkg, the installation type is @c package otherwise it is @c application
 
 Hence, the installation type in the enclosure element only needs to be specified for package based updates distributed inside of a @c zip or other archive format.
 
 Old applications must be using Sparkle 1.26 or later to support downloading bare package updates (`pkg` or `mpkg`) that are not additionally archived inside of a @c zip or other archive format.
 */
@property (nonatomic, copy, readonly) NSString *installationType;

/**
 The appcast signing validation status that this appcast item came from.
 
 Please see documentation of @c SPUAppcastSigningValidationStatus values for more information.
 */
@property (readonly, nonatomic) SPUAppcastSigningValidationStatus signingValidationStatus;

/**
 The phased rollout interval of the update item in seconds if provided.
 
 This is the interval between when different groups of users are notified of a new update.
 
 For this property to be used by Sparkle, the published `date` on the update item must be present as well.
 
 After each interval after the update item's `date`, a new group of users become eligible for being notified of the new update.
 
 This is extracted from the @c <sparkle:phasedRolloutInterval> element.
 
 Old applications must be using Sparkle 1.25 or later to support phased rollout intervals, otherwise they may assume updates are immediately available.
 */
@property (nonatomic, copy, readonly, nullable) NSNumber* phasedRolloutInterval;

/**
 The minimum bundle version string this update requires for automatically downloading and installing updates if provided.
 
 If an application's bundle version meets this version requirement, it can install the new update item in the background automatically.
 
 Otherwise if the requirement is not met, the user is always  prompted to install the update. In this case, the update is assumed to be a `majorUpgrade`.
 
 If the update is a `majorUpgrade` and the update is skipped by the user, other future update alerts with the same `minimumAutoupdateVersion` will also be skipped automatically unless an update specifies `ignoreSkippedUpgradesBelowVersion`.
 
 This version string corresponds to the application's @c CFBundleVersion
 
 This is extracted from the @c <sparkle:minimumAutoupdateVersion> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *minimumAutoupdateVersion;

/**
 Indicates whether or not the update item is a major upgrade.
 
 An update is a major upgrade if the application's bundle version doesn't meet the `minimumAutoupdateVersion` requirement.
 */
@property (nonatomic, getter=isMajorUpgrade, readonly) BOOL majorUpgrade;

/**
 Previously skipped upgrades by the user will be ignored if they skipped an update whose version precedes this version.
 
 This can only be applied if the update is a `majorUpgrade`.
 
 This version string corresponds to the application's @c CFBundleVersion
 
 This is extracted from the @c <sparkle:ignoreSkippedUpgradesBelowVersion> element.
 
 Old applications must be using Sparkle 2.1 or later, otherwise this property will be ignored.
 */
@property (nonatomic, readonly, nullable) NSString *ignoreSkippedUpgradesBelowVersion;

/**
 Indicates whether or not the update item is critical.
 
 Critical updates are shown to the user more promptly. Sparkle's standard user interface also does not allow them to be skipped.
 
 This is determined and extracted from a top-level @c <sparkle:criticalUpdate> element or a @c sparkle:criticalUpdate element inside of a @c sparkle:tags element.
 
 Old applications must be using Sparkle 2 or later to support the top-level @c <sparkle:criticalUpdate> element.
 */
@property (nonatomic, getter=isCriticalUpdate, readonly) BOOL criticalUpdate;

/**
 Specifies the operating system the download update is available for if provided.
 
 If this property is not provided, then the supported operating system is assumed to be macOS.
 
 Known potential values for this string are @c macos and @c windows
 
 Sparkle on Mac ignores update items that are for other operating systems.
 This is only useful for sharing appcasts between Sparkle on Mac and Sparkle on other operating systems.
 
 Use `macOsUpdate` property to test if this update item is for macOS.
 
 This is extracted from the @c sparkle:os attribute in the @c <enclosure> element.
 */
@property (nonatomic, copy, readonly, nullable) NSString *osString;

/**
 Indicates whether or not this update item is for macOS.
 
 This is determined from the `osString` property.
 */
@property (nonatomic, getter=isMacOsUpdate, readonly) BOOL macOsUpdate;

/**
 The delta updates for this update item.
 
 Sparkle uses these to download and apply a smaller update based on the version the user is updating from.
 
 The key is based on the @c sparkle:version of the update.
 The value is an update item that will have `deltaUpdate` be @c YES
 
 Clients typically should not need to examine the contents of the delta updates.
 
 This is extracted from the @c <sparkle:deltas> element.
 */
@property (nonatomic, copy, readonly, nullable) NSDictionary<NSString *, SUAppcastItem *> *deltaUpdates;

/**
 The expected size of the Sparkle executable file before applying this delta update.
 
 This attribute is used to test if the delta item can still be applied. If Sparkle's executable file has changed (e.g. from having an architecture stripped),
 then the delta item cannot be applied.
 
 This is extracted from the @c sparkle:deltaFromSparkleExecutableSize attribute from the @c <enclosure> element of a @c sparkle:deltas item.
 This attribute is optional for delta update items.
 */
@property (nonatomic, nonatomic, readonly, nullable) NSNumber *deltaFromSparkleExecutableSize;

/**
 An expected set of Sparkle's locales present on disk before applying this delta update.
 
 This attribute is used to test if the delta item can still be applied. If Sparkle's list of locales present on disk  (.lproj directories) do not contain any items from this set,
 (e.g. from having localization files stripped) then the delta item cannot be applied. This set does not need to be a complete list of locales. Sparkle may even decide
 to not process all them. 1-10 should be a decent amount.
 
 This is extracted from the @c sparkle:deltaFromSparkleLocales attribute from the @c <enclosure> element of a @c sparkle:deltas item.
 The locales extracted from this attribute are delimited by a comma (e.g. "en,ca,fr,hr,hu"). This attribute is optional for delta update items.
 */
@property (nonatomic, nonatomic, readonly, nullable) NSSet<NSString *> *deltaFromSparkleLocales;

/**
 Indicates whether or not the update item is a delta update.
 
 An update item is a delta update if it is in the `deltaUpdates` of another update item.
 */
@property (nonatomic, getter=isDeltaUpdate, readonly) BOOL deltaUpdate;

/**
 The dictionary representing the entire appcast item.
 
 This is useful for querying custom extensions or elements from the appcast item.
 */
@property (nonatomic, readonly, copy) NSDictionary *propertiesDictionary;

- (instancetype)init NS_UNAVAILABLE;

/**
 An empty appcast item.
 
 This may be used as a potential return value in `-[SPUUpdaterDelegate bestValidUpdateInAppcast:forUpdater:]`
 */
+ (instancetype)emptyAppcastItem;

// Deprecated initializers
- (nullable instancetype)initWithDictionary:(NSDictionary *)dict __deprecated_msg("Properties that depend on the system or application version are not supported when used with this initializer. The designated initializer is available in SUAppcastItem+Private.h. Please first explore other APIs or contact us to describe your use case.");
- (nullable instancetype)initWithDictionary:(NSDictionary *)dict failureReason:(NSString * _Nullable __autoreleasing *_Nullable)error __deprecated_msg("Properties that depend on the system or application version are not supported when used with this initializer. The designated initializer is available in SUAppcastItem+Private.h. Please first explore other APIs or contact us to describe your use case.");
- (nullable instancetype)initWithDictionary:(NSDictionary *)dict relativeToURL:(NSURL * _Nullable)appcastURL failureReason:(NSString * _Nullable __autoreleasing *_Nullable)error __deprecated_msg("Properties that depend on the system or application version are not supported when used with this initializer. The designated initializer is available in SUAppcastItem+Private.h. Please first explore other APIs or contact us to describe your use case.");

@end

NS_ASSUME_NONNULL_END

#endif
