/* PictureGLView */

#include <Cocoa/Cocoa.h>

#include "Manager.h"

@interface HBPictureGLView : NSOpenGLView

{
    HBManager        * fManager;
    HBTitle          * fTitle;

    uint8_t          * fPicture;
}

- (void) SetManager: (HBManager*) manager;
- (void) SetTitle: (HBTitle*) title;
- (void) ShowPicture: (int) picture;

@end
