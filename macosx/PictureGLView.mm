/* $Id: PictureGLView.mm,v 1.18 2005/08/01 15:10:44 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <math.h>

#include "PictureGLView.h"

static int GetAlignedSize( int size )
{
    int result = 1;
    while( result < size )
    {
        result *= 2;
    }
    return result;
}

@implementation HBPictureGLView

- (id) initWithFrame: (NSRect) frame
{
    fHasQE  = CGDisplayUsesOpenGLAcceleration( kCGDirectMainDisplay );
    fTarget = fHasQE ? GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D;

    fBuffers[0] = NULL;
    fBuffers[1] = NULL;
    fWidth      = 0;
    fHeight     = 0;

    fLastEffect = -1;
    
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

    glGenTextures( 2, fTextures );

    return self;
}

- (void) reshape
{
    NSRect bounds;
    [[self openGLContext] update];
    [[self openGLContext] makeCurrentContext];
    bounds = [self bounds];
    glViewport( 0, 0, (int) bounds.size.width,
                (int) bounds.size.height );
}

- (void) drawRect: (NSRect) rect
{
    [[self openGLContext] makeCurrentContext];

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glDisable( GL_BLEND );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if( fBuffers[0] )
    {
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();

        glEnable( fTarget );
        glBindTexture( fTarget, fTextures[0] );
        glTexImage2D( fTarget, 0, GL_RGBA, fTexWidth, fTexHeight, 0,
                      GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, fBuffers[0] );
        glTexParameteri( fTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( fTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glBegin( GL_QUADS );
        glTexCoord2f( 0.0    , 0.0     ); glVertex2f( -1.0,  1.0 );
        glTexCoord2f( 0.0    , fCoordY ); glVertex2f( -1.0, -1.0 );
        glTexCoord2f( fCoordX, fCoordY ); glVertex2f(  1.0, -1.0 );
        glTexCoord2f( fCoordX, 0.0     ); glVertex2f(  1.0,  1.0 );
        glEnd();
    }
    [[self openGLContext] flushBuffer];
}

#define FRUSTUM_NEAR   2.5
#define FRUSTUM_FAR    20.0

- (void) drawCube: (int) anim
{
    uint64_t date;
    float w, rotation, translation;
    
    w = ( anim & HB_ANIMATE_BACKWARD ) ? 1.0 : -1.0;

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glDisable( GL_BLEND );

    for( rotation = 0.0; w * rotation < 90.0;
         rotation += w * 90 * 1000 / fAnimDuration / fFrameRate )
    {
        date = hb_get_date();
        translation = - FRUSTUM_NEAR - cos( rotation * M_PI / 180 ) *
            ( 1 + w * tan( rotation * M_PI / 180 ) );

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        glFrustum( -1.0, 1.0, -1.0, 1.0, FRUSTUM_NEAR, FRUSTUM_FAR );
        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
        glTranslatef( 0.0, 0.0, translation );
        glRotatef( rotation, 0.0, 1.0, 0.0 );
     
        glBindTexture( fTarget, fTextures[0] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0    , 0.0 );     glVertex3f( -1.0,  1.0, 1.0 );
        glTexCoord2f( 0.0    , fCoordY ); glVertex3f( -1.0, -1.0, 1.0 );
        glTexCoord2f( fCoordX, fCoordY ); glVertex3f(  1.0, -1.0, 1.0 );
        glTexCoord2f( fCoordX, 0.0 );     glVertex3f(  1.0,  1.0, 1.0 );
        glEnd();
     
        glBindTexture( fTarget, fTextures[1] );
        glBegin( GL_QUADS );
        if( anim & HB_ANIMATE_FORWARD )
        {
            glTexCoord2f( 0.0,    0.0 );     glVertex3f( 1.0,  1.0,  1.0 );
            glTexCoord2f( 0.0,    fCoordY ); glVertex3f( 1.0, -1.0,  1.0 );
            glTexCoord2f( fCoordX, fCoordY ); glVertex3f( 1.0, -1.0, -1.0 );
            glTexCoord2f( fCoordX, 0.0 );     glVertex3f( 1.0,  1.0, -1.0 );
        }
        else
        {
            glTexCoord2f( 0.0,    0.0 );     glVertex3f( -1.0,  1.0, -1.0 );
            glTexCoord2f( 0.0,    fCoordY ); glVertex3f( -1.0, -1.0, -1.0 );
            glTexCoord2f( fCoordX, fCoordY ); glVertex3f( -1.0, -1.0,  1.0 );
            glTexCoord2f( fCoordX, 0.0 );     glVertex3f( -1.0,  1.0,  1.0 );
        }
        glEnd();
     
        [[self openGLContext] flushBuffer];

        hb_snooze( 1000 / fFrameRate - ( hb_get_date() - date ) );
    }

}

- (void) drawSwap: (int) anim
{
    uint64_t date;
    float w, rotation, x, z;
    
    w = ( anim & HB_ANIMATE_BACKWARD ) ? 1.0 : -1.0;

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glDisable( GL_BLEND );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -1.0, 1.0, -1.0, 1.0, FRUSTUM_NEAR, FRUSTUM_FAR );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0, 0.0, - FRUSTUM_NEAR - 1.0 );

    for( rotation = 0.0; w * rotation < 180.0;
         rotation += w * 180 * 1000 / fAnimDuration / fFrameRate )
    {
        date = hb_get_date();

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        x = 1.1 * sin( rotation * M_PI / 180 );
        z = cos( rotation * M_PI / 180 );

        glBindTexture( fTarget, fTextures[0] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0,    0.0 );     glVertex3f( -1.0 + x,  1.0, z );
        glTexCoord2f( 0.0,    fCoordY ); glVertex3f( -1.0 + x, -1.0, z );
        glTexCoord2f( fCoordX, fCoordY ); glVertex3f(  1.0 + x, -1.0, z );
        glTexCoord2f( fCoordX, 0.0 );     glVertex3f(  1.0 + x,  1.0, z );
        glEnd();
     
        glBindTexture( fTarget, fTextures[1] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0,    0.0 );     glVertex3f( -1.0 - x,  1.0, - z );
        glTexCoord2f( 0.0,    fCoordY ); glVertex3f( -1.0 - x, -1.0, - z );
        glTexCoord2f( fCoordX, fCoordY ); glVertex3f(  1.0 - x, -1.0, - z );
        glTexCoord2f( fCoordX, 0.0 );     glVertex3f(  1.0 - x,  1.0, - z );
        glEnd();
     
        [[self openGLContext] flushBuffer];

        hb_snooze( 1000 / fFrameRate - ( hb_get_date() - date ) );
    }
}

- (void) drawFade
{
    uint64_t date;
    float alpha;

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    for( alpha = 0.0; alpha < 1.0;
         alpha += 1000.0 / fAnimDuration / fFrameRate )
    {
        date = hb_get_date();

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glColor4f( 1.0, 1.0, 1.0, 1.0 - alpha );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        glBindTexture( fTarget, fTextures[0] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0,    0.0 );     glVertex2f( -1.0,  1.0 );
        glTexCoord2f( 0.0,    fCoordY ); glVertex2f( -1.0, -1.0 );
        glTexCoord2f( fCoordX, fCoordY ); glVertex2f(  1.0, -1.0 );
        glTexCoord2f( fCoordX, 0.0 );     glVertex2f(  1.0,  1.0 );
        glEnd();

        glColor4f( 1.0, 1.0, 1.0, alpha );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        glBindTexture( fTarget, fTextures[1] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0,    0.0 );     glVertex2f( -1.0,  1.0 );
        glTexCoord2f( 0.0,    fCoordY ); glVertex2f( -1.0, -1.0 );
        glTexCoord2f( fCoordX, fCoordY ); glVertex2f(  1.0, -1.0 );
        glTexCoord2f( fCoordX, 0.0 );     glVertex2f(  1.0,  1.0 );
        glEnd();

        [[self openGLContext] flushBuffer];

        hb_snooze( 1000 / fFrameRate - ( hb_get_date() - date ) );
    }
}

- (void) drawSlide: (int) anim
{
    uint64_t date;
    float foo, w;
    int left, right;
    if( anim & HB_ANIMATE_FORWARD )
    {
        left  = 0;
        right = 1;
        w     = 1.0;
    }
    else
    {
        left  = 1;
        right = 0;
        w     = -1.0;
    }

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glDisable( GL_BLEND );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    for( foo = w; foo >= -1.0 && foo <= 1.0;
         foo -= w * 2000.0 / fAnimDuration / fFrameRate )
    {
        date = hb_get_date();

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glBindTexture( fTarget, fTextures[left] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0,    0.0 );     glVertex2f( foo - 2.0,  1.0 );
        glTexCoord2f( 0.0,    fCoordY ); glVertex2f( foo - 2.0, -1.0 );
        glTexCoord2f( fCoordX, fCoordY ); glVertex2f( foo,       -1.0 );
        glTexCoord2f( fCoordX, 0.0 );     glVertex2f( foo,        1.0 );
        glEnd();

        glBindTexture( fTarget, fTextures[right] );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.0,    0.0 );     glVertex2f( foo,        1.0 );
        glTexCoord2f( 0.0,    fCoordY ); glVertex2f( foo,       -1.0 );
        glTexCoord2f( fCoordX, fCoordY ); glVertex2f( foo + 2.0, -1.0 );
        glTexCoord2f( fCoordX, 0.0 );     glVertex2f( foo + 2.0,  1.0 );
        glEnd();

        [[self openGLContext] flushBuffer];

        hb_snooze( 1000 / fFrameRate - ( hb_get_date() - date ) );
    }
}

#undef FRUSTUM_NEAR
#undef FRUSTUM_FAR

- (void) drawAnimation: (int) anim
{
    glEnable( fTarget );

    glBindTexture( fTarget, fTextures[0] );
    glTexImage2D( fTarget, 0, GL_RGBA, fTexWidth, fTexHeight, 0,
                  GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, fBuffers[1] );
    glTexParameteri( fTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( fTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glBindTexture( fTarget, fTextures[1] );
    glTexImage2D( fTarget, 0, GL_RGBA, fTexWidth, fTexHeight, 0,
                  GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, fBuffers[0] );
    glTexParameteri( fTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( fTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    /* Draw a random animation, just making sure we don't use the same
       effect two times in a row */
    int effect;
    do
    {
        effect = hb_get_date() % 4;
    }
    while( effect == fLastEffect );

    fAnimDuration = ( anim & HB_ANIMATE_SLOW ) ? 3000 : 600;
    fFrameRate    = 60.0;
    
    switch( effect )
    {
        case 0:
            [self drawCube: anim];
            break;
        case 1:
            [self drawSwap: anim];
            break;
        case 2:
            [self drawFade];
            break;
        case 3:
            [self drawSlide: anim];
            break;
    }

    fLastEffect = effect;
}

- (void) Display: (int) anim buffer1: (uint8_t *) buffer1
    buffer2: (uint8_t *) buffer2 width: (int) width height: (int) height
{
    [[self openGLContext] makeCurrentContext];

    if( width != fWidth || height != fHeight )
    {
        fWidth  = width;
        fHeight = height;
        if( fHasQE )
        {
            fTexWidth  = fWidth;
            fTexHeight = fHeight;
            fCoordX    = (float) fWidth;
            fCoordY    = (float) fHeight;
        }
        else
        {
            fTexWidth  = GetAlignedSize( fWidth );
            fTexHeight = GetAlignedSize( fHeight );
            fCoordX    = (float) fWidth  / (float) fTexWidth;
            fCoordY    = (float) fHeight / (float) fTexHeight;
        }
        [self clearGLContext];
        [self openGLContext];
        [self reshape];
    }

    fBuffers[0] = buffer1;
    fBuffers[1] = buffer2;

    /* Swap buffers only during the vertical retrace of the monitor.
       http://developer.apple.com/documentation/GraphicsImaging/
       Conceptual/OpenGL/chap5/chapter_5_section_44.html */
    #if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
    long params[] = { 1 };
    #else
    int params[] = { 1 };
    #endif
    CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval, params );

    if( !( anim & HB_ANIMATE_NONE ) )
    {
        [self drawAnimation: anim];
    }
    
    [self drawRect: [self bounds]];
}

@end
