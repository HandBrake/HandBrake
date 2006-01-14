/* $Id: PictureGLView.mm,v 1.6 2004/03/08 12:39:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <math.h>

#include "PictureGLView.h"

#define PROUT 2.5

/* XXX This file needs some serious cleaning XXX */

GLuint    texture[2];
float     rotation;
float     translation;
uint8_t * truc;

@implementation HBPictureGLView

- (void) SetHandle: (HBHandle*) handle
{
    fHandle = handle;
}

- (void) SetTitle: (HBTitle*) title
{
    fTitle = title;

    /* This is needed as the view's size may have changed */
    [self clearGLContext];
    [self openGLContext];
}

- (void) ShowPicture: (int) index animate: (int) how
{
    if( fOldPicture ) free( fOldPicture );
    fOldPicture = fPicture;

    /* Get the picture */
    uint8_t * tmp = HBGetPreview( fHandle, fTitle, index );

    /* Make it be upside-down */
    fPicture = (uint8_t*) malloc( 4 * ( fTitle->outWidthMax + 2 ) *
                                  ( fTitle->outHeightMax + 2 ) );
    uint8_t * in  = tmp;
    uint8_t * out = fPicture +
        4 * ( fTitle->outWidthMax + 2 ) * ( fTitle->outHeightMax + 1 );
    for( int i = fTitle->outHeightMax + 2; i--; )
    {
        memcpy( out, in, 4 * ( fTitle->outWidthMax + 2 ) );
        in  += 4 * ( fTitle->outWidthMax + 2 );
        out -= 4 * ( fTitle->outWidthMax + 2 );
    }
    free( tmp );

    /* ARGB -> RGBA */
    uint32_t * p = (uint32_t*) fPicture;
    for( int i = ( fTitle->outHeightMax + 2 ) *
            ( fTitle->outWidthMax + 2 ); i--; )
    {
        *(p++) = ( ( (*p) & 0x00FFFFFF ) << 8 ) | 0xFF;
    }

    if( how == HB_ANIMATE_NONE )
    {
        [self drawRect: [self bounds]];
        return;
    }

    in  = fOldPicture;
    out = truc;
    for( int i = fTitle->outHeightMax + 2; i--;  )
    {
        memcpy( out, in, ( fTitle->outWidthMax + 2 ) * 4 );
        in  += ( fTitle->outWidthMax + 2 ) * 4;
        out += 1024 * 4;
    }
    glBindTexture( GL_TEXTURE_2D, texture[0] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1024,
                  1024, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, truc );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    in  = fPicture;
    out = truc;
    for( int i = fTitle->outHeightMax + 2; i--; )
    {
        memcpy( out, in, ( fTitle->outWidthMax + 2 ) * 4 );
        in  += ( fTitle->outWidthMax + 2 ) * 4;
        out += 1024 * 4;
    }
    glBindTexture( GL_TEXTURE_2D, texture[1] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1024,
                 1024, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, truc );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glEnable( GL_TEXTURE_2D );
    glShadeModel( GL_SMOOTH );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.5f );
    glClearDepth( 1.0f );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

#define ANIMATION_TIME 500000
#define FRAME_PER_SEC  50

    rotation = 0.0;
    float w = ( how == HB_ANIMATE_LEFT ) ? 1.0 : -1.0;
    uint64_t date;
    int64_t  wait;
    for( ;; )
    {
        date = HBGetDate();
        translation = - PROUT - cos( rotation * M_PI / 180 ) *
                             ( 1 + w * tan( rotation * M_PI / 180 ) );
        [self drawAnimation: how];

        rotation += w * 90 * 1000000 / ANIMATION_TIME / FRAME_PER_SEC;
        if( w * rotation >= 90.0 )
        {
            break;
        }

        wait = 1000000 / FRAME_PER_SEC - ( HBGetDate() - date );
        if( wait > 0 )
        {
            HBSnooze( wait );
        }
    }

    [self drawRect: [self bounds]];
}

- (id) initWithFrame: (NSRect) frame
{
    fHandle    = NULL;
    fTitle      = NULL;
    fPicture    = NULL;
    fOldPicture = NULL;

    GLuint attribs[] =
    {
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAWindow,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAStencilSize, 8,
        NSOpenGLPFAAccumSize, 0,
        0
    };

    NSOpenGLPixelFormat * fmt = [[NSOpenGLPixelFormat alloc]
        initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs];

    self = [super initWithFrame:frame pixelFormat: [fmt autorelease]];

    if( !self )
    {
        return NULL;
    }

    [[self openGLContext] makeCurrentContext];
    [self reshape];


    glGenTextures( 2, texture );
    truc = (uint8_t*) malloc( 1024*1024*4 );

    return self;
}

/*
 * Resize ourself
 */
- (void) reshape
{
   NSRect bounds;

   [[self openGLContext] update];
   bounds = [self bounds];
   if( fTitle )
   {
       glViewport( 0, 0, fTitle->outWidthMax + 2,
                   fTitle->outHeightMax + 2 );
   }
}

- (void) drawAnimation: (int) how
{
    /* Swap buffers only during the vertical retrace of the monitor.
       http://developer.apple.com/documentation/GraphicsImaging/
       Conceptual/OpenGL/chap5/chapter_5_section_44.html */
    long params[] = { 1 };
    CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval,
                     params );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -1.0, 1.0, -1.0, 1.0, PROUT, 20.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0, 0.0, translation );
    glRotatef( rotation, 0.0, 1.0, 0.0 );
 
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
 
    glBindTexture( GL_TEXTURE_2D, texture[0] );
 
    glBegin( GL_QUADS );
    glTexCoord2f( 0.0, 0.0 );
    glVertex3f( -1.0, -1.0,  1.0 );
    glTexCoord2f( ( 2.0 + fTitle->outWidthMax ) / 1024, 0.0 );
    glVertex3f(  1.0, -1.0,  1.0 );
    glTexCoord2f( ( 2.0 + fTitle->outWidthMax ) / 1024,
                  ( 2.0 + fTitle->outHeightMax ) / 1024 );
    glVertex3f(  1.0,  1.0,  1.0 );
    glTexCoord2f( 0.0, ( 2.0 + fTitle->outHeightMax ) / 1024 );
    glVertex3f( -1.0,  1.0,  1.0 );
    glEnd();
 
    glBindTexture( GL_TEXTURE_2D, texture[1] );
 
    glBegin( GL_QUADS );
    if( how == HB_ANIMATE_RIGHT )
    {
        glTexCoord2f( 0.0, 0.0 );
        glVertex3f(  1.0, -1.0,  1.0 );
        glTexCoord2f( ( 2.0 + fTitle->outWidthMax ) / 1024, 0.0 );
        glVertex3f(  1.0, -1.0,  -1.0 );
        glTexCoord2f( ( 2.0 + fTitle->outWidthMax ) / 1024,
                      ( 2.0 + fTitle->outHeightMax ) / 1024 );
        glVertex3f(  1.0,  1.0,  -1.0 );
        glTexCoord2f( 0.0, ( 2.0 + fTitle->outHeightMax ) / 1024 );
        glVertex3f(  1.0,  1.0,  1.0 );
    }
    else
    {
        glTexCoord2f( 0.0, 0.0 );
        glVertex3f(  -1.0, -1.0, -1.0 );
        glTexCoord2f( ( 2.0 + fTitle->outWidthMax ) / 1024, 0.0 );
        glVertex3f(  -1.0, -1.0, 1.0 );
        glTexCoord2f( ( 2.0 + fTitle->outWidthMax ) / 1024,
                      ( 2.0 + fTitle->outHeightMax ) / 1024 );
        glVertex3f(  -1.0,  1.0, 1.0 );
        glTexCoord2f( 0.0, ( 2.0 + fTitle->outHeightMax ) / 1024 );
        glVertex3f(  -1.0,  1.0, -1.0 );
    }
    glEnd();
 
    [[self openGLContext] flushBuffer];
}

- (void) drawRect: (NSRect) rect
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   if( !fPicture )
   {
       return;
   }

   glDrawPixels( fTitle->outWidthMax + 2,
                 fTitle->outHeightMax + 2, GL_RGBA,
                 GL_UNSIGNED_BYTE, fPicture );

   [[self openGLContext] flushBuffer];
}

@end
