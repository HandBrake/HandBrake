/* $Id: Mpeg2Dec.c,v 1.15 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "mpeg2dec/mpeg2.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle           * handle;
    HBTitle            * title;
    int                  pass;
    mpeg2dec_t         * libmpeg2;
    const mpeg2_info_t * info;
    int                  lateField;
};

/* Local prototypes */
static int Mpeg2DecWork( HBWork * );

HBWork * HBMpeg2DecInit( HBHandle * handle, HBTitle * title )
{
    HBWork * w ;
    if( !( w = malloc( sizeof( HBWork ) ) ) )
    {
        HBLog( "HBMpeg2Dec: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name   = strdup( "Mpeg2Dec" );
    w->work   = Mpeg2DecWork;

    w->handle = handle;
    w->title  = title;

    w->pass          = 42;
    w->libmpeg2      = NULL;
    w->info          = NULL;
    w->lateField     = 0;

    return w;
}

void HBMpeg2DecClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->libmpeg2 )
    {
        HBLog( "HBMpeg2Dec: closing libmpeg2 (pass %d)", w->pass );
        mpeg2_close( w->libmpeg2 );
    }
    free( w->name );
    free( w );

    *_w = NULL;
}

static int Mpeg2DecWork( HBWork * w )
{
    HBTitle     * title = w->title;
    HBBuffer    * mpeg2Buffer;
    HBBuffer    * rawBuffer;
    mpeg2_state_t state;

    if( HBFifoIsHalfFull( title->rawFifo ) )
    {
        return 0;
    }

    /* Get a new buffer to decode */
    if( !( mpeg2Buffer = HBFifoPop( title->inFifo ) ) )
    {
        return 0;
    }

    /* Init or re-init if needed */
    if( mpeg2Buffer->pass != w->pass )
    {
        if( w->libmpeg2 )
        {
            HBLog( "HBMpeg2Dec: closing libmpeg2 (pass %d)", w->pass );
            mpeg2_close( w->libmpeg2 );
        }

        w->pass = mpeg2Buffer->pass;

        HBLog( "HBMpeg2Dec: opening libmpeg2 (pass %d)", w->pass );
#ifdef HB_NOMMX
        mpeg2_accel( 0 );
#endif
        w->libmpeg2  = mpeg2_init();
        w->info      = mpeg2_info( w->libmpeg2 );
        w->lateField = 0;
    }

    /* Decode */
    mpeg2_buffer( w->libmpeg2, mpeg2Buffer->data,
                  mpeg2Buffer->data + mpeg2Buffer->size );

    for( ;; )
    {
        state = mpeg2_parse( w->libmpeg2 );

        if( state == STATE_BUFFER )
        {
            break;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 w->info->display_fbuf )
        {
            rawBuffer = HBBufferInit( 3 * title->inWidth *
                                      title->inHeight );

            /* TODO: make libmpeg2 write directly in our buffer */
            memcpy( rawBuffer->data, w->info->display_fbuf->buf[0],
                    title->inWidth * title->inHeight );
            memcpy( rawBuffer->data + title->inWidth * title->inHeight,
                    w->info->display_fbuf->buf[1],
                    title->inWidth * title->inHeight / 4 );
            memcpy( rawBuffer->data + title->inWidth * title->inHeight +
                        title->inWidth * title->inHeight / 4,
                    w->info->display_fbuf->buf[2],
                    title->inWidth * title->inHeight / 4 );

            rawBuffer->position = mpeg2Buffer->position;
            rawBuffer->pass     = mpeg2Buffer->pass;

            /* NTSC pulldown kludge */
            if( w->info->display_picture->nb_fields == 3 )
            {
                rawBuffer->repeat = w->lateField;
                w->lateField      = !w->lateField;
            }
            else
            {
                rawBuffer->repeat = 0;
            }

            if( !HBFifoPush( title->rawFifo, &rawBuffer ) )
            {
                HBLog( "HBMpeg2Dec: HBFifoPush failed" );
            }
        }
        else if( state == STATE_INVALID )
        {
            /* Shouldn't happen on a DVD */
            HBLog( "HBMpeg2Dec: STATE_INVALID" );
        }
    }

    HBBufferClose( &mpeg2Buffer );

    return 1;
}
