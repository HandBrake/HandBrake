/* $Id: XvidEnc.c,v 1.5 2003/11/05 19:14:37 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "XvidEnc.h"
#include "Fifo.h"
#include "Work.h"

#include <xvid.h>

/* Local prototypes */
static int XvidEncWork( HBWork * );

struct HBXvidEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle * handle;
    HBTitle  * title;

    void     * xvid;
    HBBuffer * mpeg4Buffer;
    int        pass;
};

HBXvidEnc * HBXvidEncInit( HBHandle * handle, HBTitle * title )
{
    HBXvidEnc * x;
    if( !( x = malloc( sizeof( HBXvidEnc ) ) ) )
    {
        HBLog( "HBXvidEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    x->name = strdup( "XvidEnc" );
    x->work = XvidEncWork;

    x->handle = handle;
    x->title = title;

    x->xvid        = NULL;
    x->mpeg4Buffer = NULL;
    x->pass        = 42;

    return x;
}

void HBXvidEncClose( HBXvidEnc ** _x )
{
    HBXvidEnc * x = *_x;
    free( x );
    *_x = NULL;
}

static int XvidEncWork( HBWork * w )
{
    HBXvidEnc      * x = (HBXvidEnc*) w;
    HBTitle        * title = x->title;
    HBBuffer       * scaledBuffer;
    HBBuffer       * mpeg4Buffer;
    XVID_ENC_FRAME   xframe;

    int didSomething = 0;

    if( x->mpeg4Buffer )
    {
        if( HBFifoPush( title->mpeg4Fifo, &x->mpeg4Buffer ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
    }

    if( ( scaledBuffer = HBFifoPop( title->scaledFifo ) ) )
    {
        didSomething = 1;
    }
    else
    {
        return didSomething;
    }

    /* Init or re-init if needed */
    if( scaledBuffer->pass != x->pass )
    {
        XVID_INIT_PARAM xinit;
        XVID_ENC_PARAM xparam;

        x->pass = scaledBuffer->pass;;

        HBLog( "HBXvidEnc: opening libxvidcore (pass %d)", x->pass );

        xinit.cpu_flags = 0;
        xvid_init( NULL, 0, &xinit, NULL );

        xparam.width  = title->outWidth;
        xparam.height = title->outHeight;
        
        xparam.fincr  = title->rateBase;
        xparam.fbase  = title->rate;

        xparam.rc_bitrate = title->bitrate * 1000;

        /* Default values should be ok */
        xparam.rc_reaction_delay_factor = -1;
        xparam.rc_averaging_period      = -1;
        xparam.rc_buffer                = -1;
        xparam.max_quantizer            = -1;
        xparam.min_quantizer            = -1;
        xparam.max_key_interval         = -1;

        if( xvid_encore( NULL, XVID_ENC_CREATE, &xparam, NULL ) )
        {
            HBLog( "HBXvidEnc: xvid_encore() failed" );
        }

        x->xvid = xparam.handle;
    }

    /* TODO implement 2-pass encoding */
    if( x->pass == 1 )
    {
        HBPosition( x->handle, scaledBuffer->position );
        HBBufferClose( &scaledBuffer );
        return didSomething;
    }

    mpeg4Buffer = HBBufferInit( title->outWidth *
                                title->outHeight * 3 / 2 );
    mpeg4Buffer->position = scaledBuffer->position;

    xframe.general   = XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V;
    xframe.motion    = PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 |
                       PMV_EXTSEARCH16 | PMV_EARLYSTOP8 |
                       PMV_HALFPELREFINE8 | PMV_HALFPELDIAMOND8 |
                       PMV_USESQUARES16;
    xframe.bitstream = mpeg4Buffer->data;

    xframe.image      = scaledBuffer->data;
    xframe.colorspace = XVID_CSP_I420;

    xframe.quant_intra_matrix = NULL;
    xframe.quant_inter_matrix = NULL;
    xframe.quant              = 0;
    xframe.intra              = -1;

    xframe.hint.hintstream = NULL;

    if( xvid_encore( x->xvid, XVID_ENC_ENCODE, &xframe, NULL ) )
    {
        HBLog( "HBXvidEnc: xvid_encore() failed" );
    }

    mpeg4Buffer->size     = xframe.length;
    mpeg4Buffer->keyFrame = xframe.intra;

    /* Inform the GUI about the current position */
    HBPosition( x->handle, scaledBuffer->position );

    HBBufferClose( &scaledBuffer );
    x->mpeg4Buffer = mpeg4Buffer;

    return didSomething;
}

