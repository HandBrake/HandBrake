/* $Id: XvidEnc.c,v 1.7 2003/11/09 21:26:52 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"
#include "Work.h"
#include "XvidEnc.h"
#include "XvidVbr.h"

#include <xvid.h>

/* Local prototypes */
static int XvidEncWork( HBWork * );

struct HBXvidEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle * handle;
    HBTitle  * title;

    void           * xvid;
    vbr_control_t    xvidVbr;
    XVID_ENC_FRAME   frame;
    HBBuffer       * mpeg4Buffer;
    int              pass;
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

    x->frame.general   = XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V;
    x->frame.motion    = PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 |
                         PMV_EXTSEARCH16 | PMV_EARLYSTOP8 |
                         PMV_HALFPELREFINE8 | PMV_HALFPELDIAMOND8 |
                         PMV_USESQUARES16;

    x->frame.colorspace = XVID_CSP_I420;

    x->frame.quant_intra_matrix = NULL;
    x->frame.quant_inter_matrix = NULL;

    x->mpeg4Buffer = NULL;
    x->pass        = 42;

    return x;
}

void HBXvidEncClose( HBXvidEnc ** _x )
{
    HBXvidEnc * x = *_x;

    if( x->xvid )
    {
        HBLog( "HBXvidEnc: closing libxvidcore (pass %d)",
                x->pass );

        xvid_encore( x->xvid, XVID_ENC_DESTROY, NULL, NULL);
        vbrFinish( &x->xvidVbr );
    }
    
    free( x );

    *_x = NULL;
}

static int XvidEncWork( HBWork * w )
{
    HBXvidEnc      * x = (HBXvidEnc*) w;
    HBTitle        * title = x->title;
    HBBuffer       * scaledBuffer;
    HBBuffer       * mpeg4Buffer;
    XVID_ENC_STATS   stats;

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

        if( x->xvid )
        {
            HBLog( "HBXvidEnc: closing libxvidcore (pass %d)",
                    x->pass );

            xvid_encore( x->xvid, XVID_ENC_DESTROY, NULL, NULL);
            vbrFinish( &x->xvidVbr );
        }

        x->pass = scaledBuffer->pass;;

        HBLog( "HBXvidEnc: opening libxvidcore (pass %d)", x->pass );

        xinit.cpu_flags = 0;
        xvid_init( NULL, 0, &xinit, NULL );

        xparam.width  = title->outWidth;
        xparam.height = title->outHeight;

        xparam.fincr  = title->rateBase;
        xparam.fbase  = title->rate;

        xparam.rc_bitrate = title->bitrate * 1024;

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

        /* Init VBR engine */
        vbrSetDefaults( &x->xvidVbr );
        if( !x->pass )
        {
            x->xvidVbr.mode = VBR_MODE_1PASS;
        }
        else if( x->pass == 1 )
        {
            x->xvidVbr.mode = VBR_MODE_2PASS_1;
        }
        else
        {
            x->xvidVbr.mode = VBR_MODE_2PASS_2;
        }
        x->xvidVbr.fps = (double) title->rate / title->rateBase;
        x->xvidVbr.debug = 0;
        x->xvidVbr.filename = malloc( 1024 );
        memset( x->xvidVbr.filename, 0, 1024 );
        snprintf( x->xvidVbr.filename, 1023, "/tmp/HB.%d.xvid.log",
                  HBGetPid( x->handle ) );
        x->xvidVbr.desired_bitrate = title->bitrate * 1024;
        x->xvidVbr.max_key_interval = 10 * title->rate / title->rateBase;

        vbrInit( &x->xvidVbr );
    }

    mpeg4Buffer = HBBufferInit( title->outWidth *
                                title->outHeight * 3 / 2 );
    mpeg4Buffer->position = scaledBuffer->position;

    x->frame.bitstream = mpeg4Buffer->data;
    x->frame.length    = -1;

    x->frame.image = scaledBuffer->data;

    x->frame.quant = vbrGetQuant( &x->xvidVbr );
    x->frame.intra = vbrGetIntra( &x->xvidVbr );

    x->frame.hint.hintstream = NULL;

    if( xvid_encore( x->xvid, XVID_ENC_ENCODE, &x->frame, &stats ) )
    {
        HBLog( "HBXvidEnc: xvid_encore() failed" );
    }

    vbrUpdate( &x->xvidVbr, stats.quant, x->frame.intra, stats.hlength,
               x->frame.length, stats.kblks, stats.mblks, stats.ublks );

    mpeg4Buffer->size     = x->frame.length;
    mpeg4Buffer->keyFrame = x->frame.intra;

    /* Inform the GUI about the current position */
    HBPosition( x->handle, scaledBuffer->position );

    HBBufferClose( &scaledBuffer );

    if( x->pass == 1 )
    {
        HBBufferClose( &mpeg4Buffer );
        return didSomething;
    }

    x->mpeg4Buffer = mpeg4Buffer;
    return didSomething;
}

