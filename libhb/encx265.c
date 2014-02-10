/* encx265.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#ifdef USE_X265
#include <stdarg.h>
#include <time.h>

#include "hb.h"
#include "hb_dict.h"
#include "x265.h"

int  encx265Init( hb_work_object_t *, hb_job_t * );
int  encx265Work( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encx265Close( hb_work_object_t * );
void writeNALs(hb_work_private_t *, const x265_nal *, int );

hb_work_object_t hb_encx265 =
{
    WORK_ENCX265,
    "H.265/HEVC encoder (libx265)",
    encx265Init,
    encx265Work,
    encx265Close
};

#define FRAME_INFO_MAX2 (8)     // 2^8 = 256; 90000/256 = 352 frames/sec
#define FRAME_INFO_MIN2 (17)    // 2^17 = 128K; 90000/131072 = 1.4 frames/sec
#define FRAME_INFO_SIZE (1 << (FRAME_INFO_MIN2 - FRAME_INFO_MAX2 + 1))
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

static const char * const hb_x265_encopt_synonyms[][2] =
{
    { "me",       "motion",       },
    { NULL,           NULL,           },
};

struct hb_work_private_s
{
    hb_job_t       * job;
    x265_encoder   * x265;
    x265_param     * param;
    x265_picture   pic_in;
    x265_nal       * p_nal;
    uint32_t       nal_count;
    uint8_t        * grey_data;

    uint32_t       frames_in;
    uint32_t       frames_out;
    int            chap_mark;   // saved chap mark when we're propagating it
    int64_t        last_stop;   // Debugging - stop time of previous input frame
    int64_t        next_chap;

    struct {
        int64_t duration;
    } frame_info[FRAME_INFO_SIZE];

    int i_type;
    int numEncode;
    int64_t i_pts;

    FILE *fout;
};

/***********************************************************************
 * hb_work_encx265_init
 ***********************************************************************
 *
 **********************************************************************/
int encx265Init( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    w->private_data = pv;

    pv->job = job;
    pv->fout = fopen(job->file, "wb");
    fseek(pv->fout, 0, SEEK_SET);
    
    x265_param *param = pv->param = x265_param_alloc();
   
    x265_param_default_preset(param, "medium", "psnr"); 

    hb_log("Output video resolution: %dx%d", job->width, job->height);
    param->sourceWidth = job->width;
    param->sourceHeight = job->height;
    param->frameRate = job->vrate/job->vrate_base;
    param->poolNumThreads = hb_get_cpu_count();

    param->logLevel = X265_LOG_INFO;
    param->frameNumThreads = hb_get_cpu_count();
    param->tuQTMaxInterDepth = 1;
    param->tuQTMaxIntraDepth = 1;

    hb_dict_t *x265_opts = NULL;
    if (job->advanced_opts != NULL && *job->advanced_opts != '\0')
    {   
        x265_opts = hb_encopts_to_dict(job->advanced_opts, job->vcodec);
    }   
    /* iterate through x265_opts and parse the options */
    int ret;
    hb_dict_entry_t *entry = NULL;
    while ((entry = hb_dict_next(x265_opts, entry)) != NULL)
    { 

        ret = x265_param_parse( param, entry->key, entry->value );
        /* Let x265 sanity check the options for us */
        if( ret == X265_PARAM_BAD_NAME )
            hb_log( "x265 options: Unknown suboption %s", entry->key );
        if( ret == X265_PARAM_BAD_VALUE )
            hb_log( "x265 options: Bad argument %s=%s", entry->key, entry->value ? entry->value : "(null)" );
  
    }   
    hb_dict_free(&x265_opts);

    param->subpelRefine = 1;
    param->maxNumMergeCand = 1;
    param->bEnablePsnr = 1;
    
    if (job->vquality > 0)
        param->rc.qp = (int)job->vquality; 

    param->rc.bitrate = job->vbitrate;

    x265_setup_primitives(param, 0);

    pv->x265 = x265_encoder_open( param );
    if ( pv->x265 == NULL )
    {
        hb_error("encx265: x265_encoder_open failed.");
        free( pv );
        pv = NULL;
        return 1;
    }
    pv->numEncode = 0;
    if (!x265_encoder_headers(pv->x265, &pv->p_nal, &pv->nal_count))
    {
        writeNALs(pv, pv->p_nal, pv->nal_count);
    }
    x265_picture_init(param, &pv->pic_in);
    return 0;
}

void encx265Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    x265_param_free(pv->param);
    x265_encoder_close(pv->x265);
    fclose(pv->fout);
    free(pv);
    w->private_data = NULL;
}

/*
 * see comments in definition of 'frame_info' in pv struct for description
 * of what these routines are doing.
 */
static void save_frame_info( hb_work_private_t * pv, hb_buffer_t * in )
{
    int i = (in->s.start >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    pv->frame_info[i].duration = in->s.stop - in->s.start;
}

void writeNALs(hb_work_private_t * pv, const x265_nal* nal, int nalcount)
{
    int i; 
    for (i = 0; i < nalcount; i++)
    {
        fwrite((const char*)nal->payload, 1, nal->sizeBytes,  pv->fout);
        nal++;
    }
}

static hb_buffer_t *x265_encode( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job = pv->job;

    x265_picture pic_out;
    int numEncode;

    pv->pic_in.stride[0] = in->plane[0].stride;
    pv->pic_in.stride[1] = in->plane[1].stride;
    pv->pic_in.stride[2] = in->plane[2].stride;
    pv->pic_in.planes[0] = in->plane[0].data;
    pv->pic_in.planes[1] = in->plane[1].data;
    pv->pic_in.planes[2] = in->plane[2].data;
    pv->pic_in.bitDepth = 8;
    
    if( in->s.new_chap && job->chapter_markers )
    {
        pv->i_type = X265_TYPE_IDR;
        if( pv->next_chap == 0 )
        {
            pv->next_chap = in->s.start;
            pv->chap_mark = in->s.new_chap;
        }
        in->s.new_chap = 0;
    }
    else
    {
        pv->i_type = X265_TYPE_AUTO;
    }

    if( pv->last_stop != in->s.start )
    {
        hb_log("encx265 input continuity err: last stop %"PRId64"  start %"PRId64,
                pv->last_stop, in->s.start);
    }
    pv->last_stop = in->s.stop;

    save_frame_info( pv, in );

    pv->pic_in.pts = in->s.start;
    numEncode = x265_encoder_encode( pv->x265, &pv->p_nal, &pv->nal_count, &pv->pic_in, &pic_out );
    pv->numEncode += numEncode;
    if ( pv->nal_count > 0 )
        writeNALs(pv, pv->p_nal, pv->nal_count);

    return NULL;
}

int encx265Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    int numEncode;
    
    *buf_out = NULL;
    if (in->size <= 0)
    {
        x265_picture pic_out;
        uint32_t i_nal;
        x265_nal *nal;
        x265_stats stats;
        hb_buffer_t *last_buf = NULL;
        while (1)
        {
            numEncode = x265_encoder_encode(pv->x265, &nal, &i_nal, NULL, &pic_out);
            if (i_nal <= 0)
                break;
            pv->numEncode += numEncode;
            writeNALs(pv, nal, i_nal);

        }
        // Flushed everything - add the eof to the end of the chain.
        if ( last_buf == NULL )
            *buf_out = in;
        else
            last_buf->next = in;


        *buf_in = NULL;
        
        x265_encoder_get_stats(pv->x265, &stats, sizeof(stats));  
        x265_encoder_close(pv->x265);

        if (stats.encodedPictureCount)
        {
            hb_log("X265 encoded %d frames in %.2fs (%.2f fps), %.2f kb/s, ", stats.encodedPictureCount,
                stats.elapsedEncodeTime, stats.encodedPictureCount / stats.elapsedEncodeTime, stats.bitrate);

            hb_log("Global PSNR: %.3f\n", stats.globalPsnr);
        }
        else
            hb_log("encoded 0 frames\n");

        hb_log("Work done!");
        exit(0);

        return HB_WORK_DONE;
    }

    pv->pic_in.poc = pv->frames_in;
    ++pv->frames_in;
    ++pv->frames_out;
    *buf_out = x265_encode( w, in );
    return HB_WORK_OK;
}

const char * hb_x265_encopt_name(const char *name)
{
    int i;
    for (i = 0; hb_x265_encopt_synonyms[i][0] != NULL; i++)
        if (!strcmp(name, hb_x265_encopt_synonyms[i][1]))
            return hb_x265_encopt_synonyms[i][0];
    return name;
}

#endif
