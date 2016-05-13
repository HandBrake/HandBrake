/*  HBLanguagesSelection.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/**
 *  A collection of KVO enabled model and controllers class
 *  used to populate the languages selection table view
 */

/**
 *  HBLang
 */
@interface HBLang : NSObject <NSCopying>

@property (nonatomic, readwrite) BOOL isSelected;
@property (nonatomic, readonly) NSString *language;
@property (nonatomic, readonly) NSString *iso639_2;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithLanguage:(NSString *)value iso639_2code:(NSString *)code NS_DESIGNATED_INITIALIZER;

@end;

/**
 *  HBLanguagesSelection
 */
@interface HBLanguagesSelection : NSObject

@property (nonatomic, readonly) NSMutableArray<HBLang *> *languagesArray;
@property (nonatomic, readonly) NSArray<NSString *> *selectedLanguages;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

- (instancetype)initWithLanguages:(NSArray<NSString *> *)languages NS_DESIGNATED_INITIALIZER;

@end

/**
 *  HBLanguageArrayController
 */
@interface HBLanguageArrayController : NSArrayController <NSTableViewDelegate>

/**
 *  Set whether to show only the selected languages or all languages
 */
@property (nonatomic, readwrite) BOOL showSelectedOnly;

/**
 *  Set whether the user can drag the table view's elements.
 */
@property (nonatomic, readwrite) BOOL isDragginEnabled;

@property (unsafe_unretained) IBOutlet NSTableView *tableView;

@end

NS_ASSUME_NONNULL_END
