/*  HBOutlineView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBOutlineView : NSOutlineView

/**
 *  An index set containing the indexes of the targeted rows.
 *  If the selected row indexes contain the clicked row index, it returns every selected row,
 *  otherwise it returns only the clicked row index.
 */
@property (nonatomic, readonly, copy) NSIndexSet *targetedRowIndexes;

@end

NS_ASSUME_NONNULL_END
