/*  HBLanguagesSelection.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

/**
 *  A collection of KVO enabled model and controllers class
 *  used to populate the languages selection table view
 */

/**
 *  HBLang
 */
@interface HBLang : NSObject

@property (nonatomic, readwrite) BOOL isSelected;
@property (nonatomic, readonly) NSString *language;
@property (nonatomic, readonly) NSString *iso639_2;

- (instancetype)initWithLanguage:(NSString *)value iso639_2code:(NSString *)code;

@end;

/**
 *  HBLanguagesSelection
 */
@interface HBLanguagesSelection : NSObject

@property (nonatomic, readonly) NSMutableArray *languagesArray;
@property (nonatomic, readonly) NSArray *selectedLanguages;

- (instancetype)initWithLanguages:(NSArray *)languages;

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

@property (assign) IBOutlet NSTableView *tableView;

@end
