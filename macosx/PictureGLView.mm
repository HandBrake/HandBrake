#include <OpenGL/gl.h>

#include "PictureGLView.h"

@implementation HBPictureGLView

- (void) SetManager: (HBManager*) manager
{
    fManager = manager;
}

- (void) SetTitle: (HBTitle*) title
{
    fTitle = title;
 
    /* This is needed as the view's size may have changed */
    [self clearGLContext];
    [self openGLContext];
}

- (void) ShowPicture: (int) index
{
    /* Get the picture */
    uint8_t * tmp = fManager->GetPreview( fTitle, index );

    /* Make it be upside-down */
    if( fPicture ) free( fPicture );
    fPicture = (uint8_t*) malloc( 4 * ( fTitle->fOutWidthMax + 2 ) *
                                  ( fTitle->fOutHeightMax + 2 ) );
    for( uint32_t i = 0; i < fTitle->fOutHeightMax + 2; i++ )
    {
        memcpy( fPicture + 4 * ( fTitle->fOutWidthMax + 2 ) * i,
                tmp + 4 * ( fTitle->fOutWidthMax + 2 ) *
                    ( fTitle->fOutHeightMax + 1 - i ),
                4 * ( fTitle->fOutWidthMax + 2 ) );
    }
    free( tmp );

    /* Grrr - should find a way to give ARGB to OpenGL */
    uint8_t r, g, b, a;
    for( uint32_t i = 0; i < fTitle->fOutHeightMax + 2; i++ )
    {
        for( uint32_t j = 0; j < fTitle->fOutWidthMax + 2; j++ )
        {
            a = fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)];
            r = fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)+1];
            g = fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)+2];
            b = fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)+3];

            fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)]   = r;
            fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)+1] = g;
            fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)+2] = b;
            fPicture[4*(i*(fTitle->fOutWidthMax+2)+j)+3] = a;
        }
    }

    [self setNeedsDisplay: YES];
}

/* Override NSView's initWithFrame: to specify our pixel format */
- (id) initWithFrame: (NSRect) frame
{
    fManager = NULL;
    fTitle   = NULL;
    fPicture = NULL;

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

    if( !fmt )
    {
        fprintf( stderr, "Sarass\n" );
    }

    return self = [super initWithFrame:frame pixelFormat:
                      [fmt autorelease]];
}

/* Override the view's drawRect: to draw our GL content */
- (void) drawRect: (NSRect) rect
{
    glViewport( 0, 0, (GLsizei) rect.size.width,
                (GLsizei) rect.size.height );

    /* Black background */
    glClearColor( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT );

    /* Show it */
    if( fPicture )
    {
        glDrawPixels( fTitle->fOutWidthMax + 2,
                      fTitle->fOutHeightMax + 2, GL_RGBA,
                      GL_UNSIGNED_BYTE, fPicture );
    }

    [[self openGLContext] flushBuffer];
}

@end
