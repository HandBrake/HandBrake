/* $Id: PictureGLView.h,v 1.1.1.1 2003/11/03 12:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "HandBrake.h"

#define HB_ANIMATE_NONE  0
#define HB_ANIMATE_LEFT  1
#define HB_ANIMATE_RIGHT 2

@interface HBPictureGLView : NSOpenGLView

{
    HBHandle         * fHandle;
    HBTitle          * fTitle;

    uint8_t          * fPicture;
    uint8_t          * fOldPicture;
}

- (id) initWithFrame: (NSRect) frame;
- (void) reshape;
- (void) drawRect: (NSRect) rect;
- (void) drawAnimation: (int) how;

- (void) SetHandle: (HBHandle*) handle;
- (void) SetTitle: (HBTitle*) title;
- (void) ShowPicture: (int) index animate: (int) how;

@end
