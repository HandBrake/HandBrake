/*  HBFiltersCellView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBFiltersCellView : NSTableCellView

@property (nonatomic, weak) IBOutlet NSPopUpButton *presetsPopUpButton;
@property (nonatomic, weak) IBOutlet NSPopUpButton *tunesPopUpButton;

- (IBAction)remove:(id)sender;

@end

NS_ASSUME_NONNULL_END
