/* $Id: Mpeg2Dec.c,v 1.12 2004/01/16 19:04:04 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "mpeg2dec/mpeg2.h"

typedef struct HBMpeg2Dec
{
    HB_WORK_COMMON_MEMBERS

    HBHandle           * handle;
    HBTitle            * title;
    HBList             * rawBufferList;
    int                  pass;
    mpeg2dec_t         * libmpeg2;
    const mpeg2_info_t * info;
    int                  lateField;
} HBMpeg2Dec;

/* Local prototypes */
static int Mpeg2DecWork( HBWork * );

HBWork * HBMpeg2DecInit( HBHandle * handle, HBTitle * title )
{
    HBMpeg2Dec * m ;
    if( !( m = malloc( sizeof( HBMpeg2Dec ) ) ) )
    {
        HBLog( "HBMpeg2Dec: malloc() failed, gonna crash" );
        return NULL;
    }

    m->name  = strdup( "Mpeg2Dec" );
    m->work  = Mpeg2DecWork;

    m->handle = handle;
    m->title  = title;

    m->rawBufferList = HBListInit();
    m->pass          = 42;
    m->libmpeg2      = NULL;
    m->info          = NULL;
    m->lateField     = 0;

    return (HBWork*) m;
}

void HBMpeg2DecClose( HBWork ** _m )
{
    HBBuffer * buffer;

    HBMpeg2Dec * m = (HBMpeg2Dec*) *_m;

    if( m->libmpeg2 )
    {
        HBLog( "HBMpeg2Dec: closing libmpeg2 (pass %d)", m->pass );
        mpeg2_close( m->libmpeg2 );
    }
    while( ( buffer = HBListItemAt( m->rawBufferList, 0 ) ) )
    {
        HBListRemove( m->rawBufferList, buffer );
        HBBufferClose( &buffer );
    }
    HBListClose( &m->rawBufferList );
    free( m->name );
    free( m );

    *_m = NULL;
}

static int Mpeg2DecWork( HBWork * w )
{
    HBMpeg2Dec  * m     = (HBMpeg2Dec*) w;
    HBTitle     * title = m->title;
    HBBuffer    * mpeg2Buffer;
    HBBuffer    * rawBuffer;
    HBBuffer    * tmpBuffer;
    mpeg2_state_t state;

    int didSomething = 0;

    /* Push decoded buffers */
    while( ( rawBuffer = (HBBuffer*)
                HBListItemAt( m->rawBufferList, 0 ) ) )
    {
        tmpBuffer = rawBuffer;
        if( HBFifoPush( title->rawFifo, &rawBuffer ) )
        {
            didSomething = 1;
            HBListRemove( m->rawBufferList, tmpBuffer );
        }
        else
        {
            return didSomething;
        }
    }

    /* Get a new buffer to decode */
    if( ( mpeg2Buffer = HBFifoPop( title->inFifo ) ) )
    {
        didSomething = 1;
    }
    else
    {
        return didSomething;
    }

    /* Init or re-init if needed */
    if( mpeg2Buffer->pass != m->pass )
    {
        if( m->libmpeg2 )
        {
            HBLog( "HBMpeg2Dec: closing libmpeg2 (pass %d)", m->pass );
            mpeg2_close( m->libmpeg2 );
        }

        m->pass = mpeg2Buffer->pass;

        HBLog( "HBMpeg2Dec: opening libmpeg2 (pass %d)", m->pass );
#ifdef HB_NOMMX
        mpeg2_accel( 0 );
#endif
        m->libmpeg2  = mpeg2_init();
        m->info      = mpeg2_info( m->libmpeg2 );
        m->lateField = 0;
    }

    /* Decode */
    mpeg2_buffer( m->libmpeg2, mpeg2Buffer->data,
                  mpeg2Buffer->data + mpeg2Buffer->size );

    for( ;; )
    {
        state = mpeg2_parse( m->libmpeg2 );

        if( state == STATE_BUFFER )
        {
            break;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 m->info->display_fbuf )
        {
            rawBuffer = HBBufferInit( 3 * title->inWidth *
                                      title->inHeight );

            /* TODO: make libmpeg2 write directly in our buffer */
            memcpy( rawBuffer->data, m->info->display_fbuf->buf[0],
                    title->inWidth * title->inHeight );
            memcpy( rawBuffer->data + title->inWidth * title->inHeight,
                    m->info->display_fbuf->buf[1],
                    title->inWidth * title->inHeight / 4 );
            memcpy( rawBuffer->data + title->inWidth * title->inHeight +
                        title->inWidth * title->inHeight / 4,
                    m->info->display_fbuf->buf[2],
                    title->inWidth * title->inHeight / 4 );

            rawBuffer->position = mpeg2Buffer->position;
            rawBuffer->pass     = mpeg2Buffer->pass;

            HBListAdd( m->rawBufferList, rawBuffer );

            /* NTSC pulldown kludge */
            if( m->info->display_picture->nb_fields == 3 )
            {
                rawBuffer->repeat = m->lateField;
                m->lateField      = !m->lateField;
            }
            else
            {
                rawBuffer->repeat = 0;
            }
        }
        else if( state == STATE_INVALID )
        {
            /* Shouldn't happen on a DVD */
            HBLog( "HBMpeg2Dec: STATE_INVALID" );
        }
    }

    HBBufferClose( &mpeg2Buffer );

    return didSomething;
}
