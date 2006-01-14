/* $Id: XvidEnc.c,v 1.20 2004/03/01 21:36:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "xvid.h"

typedef struct HBXvidEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle * handle;
    HBTitle  * title;

    char             file[1024];
    void           * xvid;
    xvid_enc_frame_t frame;
    HBBuffer       * mpeg4Buffer;
    int              pass;
    int              frames;
    int64_t          bytes;
} HBXvidEnc;

/* Local prototypes */
static int XvidEncWork( HBWork * );

HBWork * HBXvidEncInit( HBHandle * handle, HBTitle * title )
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

    memset( x->file, 0, 1024 );
    snprintf( x->file, 1023, "/tmp/HB.%d.xvid.log",
              HBGetPid( x->handle ) );

    x->xvid        = NULL;
    x->mpeg4Buffer = NULL;
    x->pass        = 42;
    x->frames      = 0;
    x->bytes       = 0;

    return (HBWork*) x;
}

void HBXvidEncClose( HBWork ** _x )
{
    HBXvidEnc * x = (HBXvidEnc*) *_x;

    if( x->xvid )
    {
        HBLog( "HBXvidEnc: closing libxvidcore (pass %d)",
                x->pass );
        xvid_encore( x->xvid, XVID_ENC_DESTROY, NULL, NULL);
    }
    if( x->title->esConfig )
    {
        free( x->title->esConfig );
        x->title->esConfig       = NULL;
        x->title->esConfigLength = 0;
    }
    if( x->frames )
    {
        float bitrate = (float) x->bytes * x->title->rate / x->frames /
            x->title->rateBase / 128;
        int64_t bytes = (int64_t) x->frames * x->title->bitrate * 128 *
            x->title->rateBase / x->title->rate;

        HBLog( "HBXvidEnc: %d frames encoded (%lld bytes), %.2f kbps",
               x->frames, x->bytes, bitrate );

        if( x->bytes > bytes )
        {
            HBLog( "HBXvidEnc: %lld more bytes than expected "
                   "(error=%.2f %%)", x->bytes - bytes,
                   100.0 * ( x->bytes - bytes ) / bytes );
        }
        else if( x->bytes < bytes )
        {
            HBLog( "HBXvidEnc: %lld less bytes than expected "
                   "(error=%.2f %%)", bytes - x->bytes,
                   100.0 * ( bytes - x->bytes ) / bytes );
        }
    }
    free( x->name );
    free( x );

    *_x = NULL;
}

static int XvidEncWork( HBWork * w )
{
    HBXvidEnc      * x = (HBXvidEnc*) w;
    HBTitle        * title = x->title;
    HBBuffer       * scaledBuffer;
    HBBuffer       * mpeg4Buffer;

    int didSomething = 0;

    if( x->mpeg4Buffer )
    {
        if( HBFifoPush( title->outFifo, &x->mpeg4Buffer ) )
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
        xvid_gbl_init_t xvid_gbl_init;
        xvid_enc_create_t xvid_enc_create;
        xvid_plugin_single_t single;
        xvid_plugin_2pass1_t rc2pass1;
        xvid_plugin_2pass2_t rc2pass2;
        xvid_enc_plugin_t plugins[7];

        if( x->xvid )
        {
            HBLog( "HBXvidEnc: closing libxvidcore (pass %d)",
                    x->pass );
            xvid_encore( x->xvid, XVID_ENC_DESTROY, NULL, NULL);
        }

        x->pass = scaledBuffer->pass;
        HBLog( "HBXvidEnc: opening libxvidcore (pass %d)", x->pass );

        memset( &xvid_gbl_init, 0, sizeof( xvid_gbl_init ) );
        xvid_gbl_init.version = XVID_VERSION;
        xvid_global( NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL );

        memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
        xvid_enc_create.version = XVID_VERSION;
        xvid_enc_create.width = title->outWidth;
        xvid_enc_create.height = title->outHeight;
        xvid_enc_create.zones = NULL;
        xvid_enc_create.num_zones = 0;
        xvid_enc_create.plugins = plugins;
        xvid_enc_create.num_plugins = 0;

        if( !x->pass )
        {
            memset( &single, 0, sizeof( single ) );
            single.version = XVID_VERSION;
            single.bitrate = 1024 * title->bitrate;
            plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
            plugins[xvid_enc_create.num_plugins].param = &single;
            xvid_enc_create.num_plugins++;
        }
        else if( x->pass == 1 )
        {
            memset( &rc2pass1, 0, sizeof( rc2pass1 ) );
            rc2pass1.version = XVID_VERSION;
            rc2pass1.filename = x->file;
            plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass1;
            plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
            xvid_enc_create.num_plugins++;
        }
        else if( x->pass == 2 )
        {
            memset(&rc2pass2, 0, sizeof(xvid_plugin_2pass2_t));
            rc2pass2.version = XVID_VERSION;
            rc2pass2.filename = x->file;
            rc2pass2.bitrate = 1024 * title->bitrate;
            plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass2;
            plugins[xvid_enc_create.num_plugins].param = &rc2pass2;
            xvid_enc_create.num_plugins++;
        }

        xvid_enc_create.num_threads = 0;
        xvid_enc_create.fincr = title->rateBase;
        xvid_enc_create.fbase = title->rate;
        xvid_enc_create.max_key_interval = 10 * title->rate / title->rateBase;
        xvid_enc_create.max_bframes = 0;
        xvid_enc_create.bquant_ratio = 150;
        xvid_enc_create.bquant_offset = 100;
        xvid_enc_create.frame_drop_ratio = 0;
        xvid_enc_create.global = 0;

        xvid_encore( NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL );
        x->xvid = xvid_enc_create.handle;
    }

    mpeg4Buffer = HBBufferInit( title->outWidth *
                                title->outHeight * 3 / 2 );
    mpeg4Buffer->position = scaledBuffer->position;

    memset( &x->frame, 0, sizeof( x->frame ) );
    x->frame.version = XVID_VERSION;
    x->frame.bitstream = mpeg4Buffer->data;
    x->frame.length = -1;
    x->frame.input.plane[0] = scaledBuffer->data;
    x->frame.input.csp = XVID_CSP_I420;
    x->frame.input.stride[0] = title->outWidth;
    x->frame.vol_flags = 0;
    x->frame.vop_flags = XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
                         XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED;
    x->frame.type = XVID_TYPE_AUTO;
    x->frame.quant = 0;
    x->frame.motion = XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
                      XVID_ME_EXTSEARCH16 | XVID_ME_ADVANCEDDIAMOND8 |
                      XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
                      XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP;
    x->frame.quant_intra_matrix = NULL;
    x->frame.quant_inter_matrix = NULL;

    mpeg4Buffer->size = xvid_encore( x->xvid, XVID_ENC_ENCODE,
                                     &x->frame, NULL );
    mpeg4Buffer->keyFrame = ( x->frame.out_flags & XVID_KEYFRAME );

    /* Inform the GUI about the current position */
    HBPosition( x->handle, scaledBuffer->position );

    HBBufferClose( &scaledBuffer );

    if( x->pass == 1 )
    {
        HBBufferClose( &mpeg4Buffer );
        return didSomething;
    }
    else
    {
        if( !title->esConfig )
        {
            int volStart, vopStart;
            for( volStart = 0; ; volStart++ )
            {
                if( mpeg4Buffer->data[volStart]   == 0x0 &&
                    mpeg4Buffer->data[volStart+1] == 0x0 &&
                    mpeg4Buffer->data[volStart+2] == 0x1 &&
                    mpeg4Buffer->data[volStart+3] == 0x20 )
                {
                    break;
                }
            }
            for( vopStart = volStart + 4; ; vopStart++ )
            {
                if( mpeg4Buffer->data[vopStart]   == 0x0 &&
                    mpeg4Buffer->data[vopStart+1] == 0x0 &&
                    mpeg4Buffer->data[vopStart+2] == 0x1 &&
                    mpeg4Buffer->data[vopStart+3] == 0xB6 )
                {
                    break;
                }
            }

            HBLog( "XvidEnc: VOL size is %d bytes", vopStart - volStart );
            title->esConfig = malloc( vopStart - volStart );
            title->esConfigLength = vopStart - volStart;
            memcpy( title->esConfig, mpeg4Buffer->data + volStart,
                    vopStart - volStart );
        }
        x->frames++;
        x->bytes       += mpeg4Buffer->size;
        x->mpeg4Buffer  = mpeg4Buffer;
    }

    return didSomething;
}

