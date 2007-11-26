/* $Id: PictureGLView.h,v 1.7 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "hb.h"

#define HB_ANIMATE_NONE     1
#define HB_ANIMATE_BACKWARD 2
#define HB_ANIMATE_FORWARD  4
#define HB_ANIMATE_SLOW     8

@interface HBPictureGLView : NSOpenGLView

{
    bool            fHasQE;
    unsigned long   fTarget;

    int             fWidth;
    int             fHeight;
    int             fTexWidth;
    int             fTexHeight;
    float           fCoordX;
    float           fCoordY;

    uint8_t       * fBuffers[2];
    /* Tiger */
    //unsigned long   fTextures[2];
    /* Leopard */
    unsigned int   fTextures[2];

    int             fLastEffect;
    int             fAnimDuration;
    int             fFrameRate;
}

- (id) initWithFrame: (NSRect) frame;
- (void) reshape;
- (void) drawRect: (NSRect) rect;
- (void) drawAnimation: (int) anim;

- (void) Display: (int) anim buffer1: (uint8_t *) buffer1
    buffer2: (uint8_t *) buffer2 width: (int) width
    height: (int) height;

@end
