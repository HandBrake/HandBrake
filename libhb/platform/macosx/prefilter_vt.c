/* prefilter_vt.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"

struct hb_filter_private_s
{
    hb_filter_init_t input;
    hb_filter_init_t output;

    hb_list_t *list_filter;
    hb_fifo_t *fifo_first;
    hb_fifo_t *fifo_last;

    struct
    {
        int width;
        int height;
        int rotation;
        int pix_fmt;
    } resample;

    int resample_needed;
    int done;
};

static int prefilter_vt_init(hb_filter_object_t *filter,
                              hb_filter_init_t   *init);

static int prefilter_vt_work(hb_filter_object_t *filter,
                              hb_buffer_t **buf_in,
                              hb_buffer_t **buf_out);

static void prefilter_vt_close(hb_filter_object_t *filter);

static const char prefilter_vt_template[] = "";

hb_filter_object_t hb_filter_prefilter_vt =
{
    .id                = HB_FILTER_PRE_VT,
    .enforce_order     = 1,
    .name              = "Prefilter (VideoToolbox)",
    .settings          = NULL,
    .init              = prefilter_vt_init,
    .work              = prefilter_vt_work,
    .close             = prefilter_vt_close,
    .settings_template = prefilter_vt_template,
};

static int prefilter_vt_init(hb_filter_object_t *filter, hb_filter_init_t *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("prefilter_vt: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;

    pv->input = *init;
    pv->output = *init;

    return 0;
}

static void close_filters(hb_filter_private_t *pv)
{
    if (pv->list_filter)
    {
        hb_filter_object_t *filter;
        while ((filter = hb_list_item(pv->list_filter, 0)))
        {
            if (filter->thread != NULL)
            {
                hb_thread_close(&filter->thread);
            }
            filter->close(filter);

            if (!filter->skip)
            {
                hb_fifo_close(&filter->fifo_out);
            }

            hb_list_rem(pv->list_filter, filter);
            hb_filter_close(&filter);
        }
    }

    hb_fifo_close(&pv->fifo_first);
    hb_list_close(&pv->list_filter);
}

static void prefilter_vt_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    close_filters(pv);
    free(pv);
    filter->private_data = NULL;
}

static void set_properties(hb_filter_private_t *pv, hb_buffer_t *in)
{
    if (pv->input.job->title->rotation == HB_ROTATION_90 ||
        pv->input.job->title->rotation == HB_ROTATION_270)
    {
        pv->input.geometry.width  = in->f.height;
        pv->input.geometry.height = in->f.width;
    }
    else
    {
        pv->input.geometry.width  = in->f.width;
        pv->input.geometry.height = in->f.height;
    }

    if (in->storage_type == AVFRAME)
    {
        AVFrame *frame = (AVFrame *)in->storage;
        // AVFrame format contains the hw format when using an hw decoder,
        // so extract the actual pixel format from the hw frames context
        if (frame->hw_frames_ctx)
        {
            AVHWFramesContext *frames_ctx = (AVHWFramesContext *)frame->hw_frames_ctx->data;
            pv->input.pix_fmt    = frames_ctx->sw_format;
        }
        else
        {
            pv->input.pix_fmt    = frame->format;
        }
    }
    else
    {
        pv->input.pix_fmt = in->f.fmt;
    }
}

static void process_filter(hb_filter_object_t *filter)
{
    hb_buffer_t *in, *out;

    in = hb_fifo_get_wait(filter->fifo_in);
    if (in == NULL)
    {
        return;
    }

    out = NULL;
    filter->status = filter->work(filter, &in, &out);
    if (in != NULL)
    {
        hb_buffer_close(&in);
    }
    if (out != NULL)
    {
        hb_fifo_push(filter->fifo_out, out);
    }
}

static hb_buffer_t * filter_buf(hb_filter_private_t *pv, hb_buffer_t *in)
{
    // Feed preview frame to filter chain
    hb_fifo_push(pv->fifo_first, in);

    // Process the preview frame through all filters
    for (int ii = 0; ii < hb_list_count(pv->list_filter); ii++)
    {
        hb_filter_object_t *filter = hb_list_item(pv->list_filter, ii);
        if (!filter->skip)
        {
            process_filter(filter);
        }
    }
    // Retrieve the filtered preview frame
    return hb_fifo_get(pv->fifo_last);
}

static int update(hb_filter_private_t *pv)
{
    int resample_changed;
    hb_job_t *job = pv->input.job;

    pv->resample_needed =
        (pv->output.geometry.width  != pv->input.geometry.width  ||
         pv->output.geometry.height != pv->input.geometry.height ||
         pv->output.pix_fmt   != pv->input.pix_fmt ||
         job->title->rotation != HB_ROTATION_0);

    resample_changed =
        (pv->resample_needed &&
         (pv->resample.width    != pv->input.geometry.width  ||
          pv->resample.height   != pv->input.geometry.height ||
          pv->resample.pix_fmt  != pv->input.pix_fmt ||
          pv->resample.rotation != job->title->rotation));

    if (resample_changed || (pv->resample_needed &&
                             pv->list_filter == NULL))
    {
        close_filters(pv);
        hb_list_t *list_filter  = hb_list_init();
        hb_filter_init_t init = pv->input;

        hb_filter_object_t *filter;

        // Rotate
        if (job->title->rotation != HB_ROTATION_0)
        {
            filter = hb_filter_init(HB_FILTER_ROTATE_VT);
            filter->settings = hb_dict_init();

            switch (job->title->rotation)
            {
                case HB_ROTATION_90:
                    hb_dict_set(filter->settings, "angle", hb_value_string("270"));
                    hb_log("prefilter_vt: auto-rotating video 90 degrees");
                    break;
                case HB_ROTATION_180:
                    hb_dict_set(filter->settings, "angle", hb_value_string("180"));
                    hb_log("prefilter_vt: auto-rotating video 180 degrees");
                    break;
                case HB_ROTATION_270:
                    hb_dict_set(filter->settings, "angle", hb_value_string("90"));
                    hb_log("prefilter_vt: auto-rotating video 270 degrees");
                    break;
                default:
                    hb_log("prefilter_vt: reinit_video_filters: unknown rotation, failed");
            }

            hb_list_add(list_filter, filter);
            if (filter->init != NULL && filter->init(filter, &init))
            {
                hb_error("prefilter_vt: failure to initialize filter '%s'", filter->name);
                hb_list_rem(list_filter, filter);
                hb_filter_close(&filter);
            }
        }

        // Crop Scale & Format
        if (pv->output.geometry.width  != pv->input.geometry.width  ||
            pv->output.geometry.height != pv->input.geometry.height ||
            pv->output.pix_fmt         != pv->input.pix_fmt)
        {
            filter = hb_filter_init(HB_FILTER_CROP_SCALE_VT);
            filter->settings = hb_dict_init();

            hb_dict_set_int(filter->settings, "width",  pv->output.geometry.width);
            hb_dict_set_int(filter->settings, "height", pv->output.geometry.height);

            if (pv->output.geometry.width  != pv->input.geometry.width  ||
                pv->output.geometry.height != pv->input.geometry.height)
            {
                hb_log("prefilter_vt: auto-scaling video from %d x %d",
                       pv->input.geometry.width,
                       pv->input.geometry.height);
            }

            if (pv->output.pix_fmt != pv->input.pix_fmt)
            {
                hb_dict_set_int(filter->settings, "format", pv->output.pix_fmt);
                hb_log("prefilter_vt: converting video pixel format from %s", av_get_pix_fmt_name(pv->input.pix_fmt));
            }

            hb_list_add(list_filter, filter);
            if (filter->init != NULL && filter->init(filter, &init))
            {
                hb_error("prefilter_vt: failure to initialize filter '%s'", filter->name);
                hb_list_rem(list_filter, filter);
                hb_filter_close(&filter);
            }
        }

        for (int ii = 0; ii < hb_list_count(list_filter);)
        {
            filter = hb_list_item(list_filter, ii);
            filter->done = &pv->done;
            if (filter->post_init != NULL && filter->post_init(filter, job))
            {
                hb_log("prefilter_vt: failure to initialise filter '%s'", filter->name );
                hb_list_rem(list_filter, filter);
                hb_filter_close(&filter);
                continue;
            }
            ii++;
        }

        // Set up filter fifos
        hb_fifo_t *fifo_in, *fifo_first, *fifo_last;

        fifo_last = fifo_in = fifo_first = hb_fifo_init(1, 1);
        for (int ii = 0; ii < hb_list_count(list_filter); ii++)
        {
            filter = hb_list_item(list_filter, ii);
            if (!filter->skip)
            {
                filter->fifo_in = fifo_in;
                filter->fifo_out = hb_fifo_init(1, 1);
                fifo_last = fifo_in = filter->fifo_out;
            }
        }
        pv->fifo_first = fifo_first;
        pv->fifo_last  = fifo_last;
        pv->list_filter = list_filter;

        pv->resample.width        = pv->input.geometry.width;
        pv->resample.height       = pv->input.geometry.height;
        pv->resample.rotation     = pv->output.job->title->rotation;
        pv->resample.pix_fmt      = pv->input.pix_fmt;
    }

    return 0;
}

static int prefilter_vt_work(hb_filter_object_t *filter,
                             hb_buffer_t **buf_in,
                             hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        if (pv->resample_needed)
        {
            filter_buf(pv, in);
        }
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    set_properties(pv, in);
    update(pv);

    if (pv->resample_needed)
    {
        *buf_out = filter_buf(pv, in);
    }
    else
    {
        *buf_out = in;
    }
    *buf_in = NULL;

    return HB_FILTER_OK;
}
