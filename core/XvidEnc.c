/* $Id: XvidEnc.c,v 1.26 2004/05/12 17:21:24 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "xvid.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle * handle;
    HBTitle  * title;

    char             file[1024];
    void           * xvid;
    xvid_enc_frame_t frame;
    int              pass;
};

/* Local prototypes */
static int XvidEncWork( HBWork * );

HBWork * HBXvidEncInit( HBHandle * handle, HBTitle * title )
{
    HBWork * w;
    if( !( w = malloc( sizeof( HBWork ) ) ) )
    {
        HBLog( "HBXvidEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "XvidEnc" );
    w->work = XvidEncWork;

    w->handle = handle;
    w->title  = title;

    memset( w->file, 0, 1024 );
#ifndef HB_CYGWIN
    snprintf( w->file, 1023, "/tmp/HB.%d.xvid.log",
              HBGetPid( w->handle ) );
#else
    snprintf( w->file, 1023, "C:\\HB.%d.xvid.log",
              HBGetPid( w->handle ) );
#endif

    w->xvid        = NULL;
    w->pass        = 42;

    return w;
}

void HBXvidEncClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->xvid )
    {
        HBLog( "HBXvidEnc: closing libxvidcore (pass %d)",
                w->pass );
        xvid_encore( w->xvid, XVID_ENC_DESTROY, NULL, NULL);
    }
    if( w->title->esConfig )
    {
        free( w->title->esConfig );
        w->title->esConfig       = NULL;
        w->title->esConfigLength = 0;
    }

    free( w->name );
    free( w );
    *_w = NULL;
}

static int XvidEncWork( HBWork * w )
{
    HBTitle        * title = w->title;
    HBBuffer       * scaledBuffer;
    HBBuffer       * mpeg4Buffer;

    if( HBFifoIsHalfFull( title->outFifo ) )
    {
        return 0;
    }

    if( !( scaledBuffer = HBFifoPop( title->scaledFifo ) ) )
    {
        return 0;
    }

    /* Init or re-init if needed */
    if( scaledBuffer->pass != w->pass )
    {
        xvid_gbl_init_t xvid_gbl_init;
        xvid_enc_create_t xvid_enc_create;
        xvid_plugin_single_t single;
        xvid_plugin_2pass1_t rc2pass1;
        xvid_plugin_2pass2_t rc2pass2;
        xvid_enc_plugin_t plugins[7];

        if( w->xvid )
        {
            HBLog( "HBXvidEnc: closing libxvidcore (pass %d)",
                    w->pass );
            xvid_encore( w->xvid, XVID_ENC_DESTROY, NULL, NULL);
        }

        w->pass = scaledBuffer->pass;
        HBLog( "HBXvidEnc: opening libxvidcore (pass %d)", w->pass );

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

        if( !w->pass )
        {
            memset( &single, 0, sizeof( single ) );
            single.version = XVID_VERSION;
            single.bitrate = 1024 * title->bitrate;
            plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
            plugins[xvid_enc_create.num_plugins].param = &single;
            xvid_enc_create.num_plugins++;
        }
        else if( w->pass == 1 )
        {
            memset( &rc2pass1, 0, sizeof( rc2pass1 ) );
            rc2pass1.version = XVID_VERSION;
            rc2pass1.filename = w->file;
            plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass1;
            plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
            xvid_enc_create.num_plugins++;
        }
        else if( w->pass == 2 )
        {
            memset(&rc2pass2, 0, sizeof(xvid_plugin_2pass2_t));
            rc2pass2.version = XVID_VERSION;
            rc2pass2.filename = w->file;
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
        w->xvid = xvid_enc_create.handle;
    }

    mpeg4Buffer = HBBufferInit( title->outWidth *
                                title->outHeight * 3 / 2 );
    mpeg4Buffer->position = scaledBuffer->position;

    memset( &w->frame, 0, sizeof( w->frame ) );
    w->frame.version = XVID_VERSION;
    w->frame.bitstream = mpeg4Buffer->data;
    w->frame.length = -1;
    w->frame.input.plane[0] = scaledBuffer->data;
    w->frame.input.csp = XVID_CSP_I420;
    w->frame.input.stride[0] = title->outWidth;
    w->frame.vol_flags = 0;
    w->frame.vop_flags = XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
                         XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED;
    w->frame.type = XVID_TYPE_AUTO;
    w->frame.quant = 0;
    w->frame.motion = XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
                      XVID_ME_EXTSEARCH16 | XVID_ME_ADVANCEDDIAMOND8 |
                      XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
                      XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP;
    w->frame.quant_intra_matrix = NULL;
    w->frame.quant_inter_matrix = NULL;

    mpeg4Buffer->size = xvid_encore( w->xvid, XVID_ENC_ENCODE,
                                     &w->frame, NULL );
    mpeg4Buffer->keyFrame = ( w->frame.out_flags & XVID_KEYFRAME );

    /* Inform the GUI about the current position */
    HBPosition( w->handle, scaledBuffer->position );

    HBBufferClose( &scaledBuffer );

    if( w->pass == 1 )
    {
        HBBufferClose( &mpeg4Buffer );
        return 1;
    }

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

    if( !HBFifoPush( title->outFifo, &mpeg4Buffer ) )
    {
        HBLog( "HBXvidEnc: HBFifoPush failed" );
    }

    return 1;
}

