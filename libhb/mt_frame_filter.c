/* mt_frame_filter.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* This is a pseudo-filter that wraps other filters to provide frame
 * based multi-threading of the wrapped filter. The sub-filter must
 * operate on each frame independently with no context carried over
 * from one frame to the next. */

#include "handbrake/handbrake.h"
#include "handbrake/taskset.h"

typedef struct
{
    taskset_thread_arg_t arg;
    hb_filter_private_t *pv;
    hb_buffer_t *out;
} mt_frame_thread_arg_t;

struct hb_filter_private_s
{
    hb_filter_object_t     * sub_filter;
    hb_buffer_t           ** buf;
    int                      frame_count;
    taskset_t                taskset;
    int                      thread_count;
    mt_frame_thread_arg_t ** thread_data;
};

static int mt_frame_init(hb_filter_object_t *filter, hb_filter_init_t *init);
static int mt_frame_work(hb_filter_object_t *filter,
                         hb_buffer_t **buf_in,
                         hb_buffer_t **buf_out);
static void mt_frame_close(hb_filter_object_t *filter);

static void mt_frame_filter_work(void *);

static const char mt_frame_template[] = "";

hb_filter_object_t hb_filter_mt_frame =
{
    .id                = HB_FILTER_MT_FRAME,
    .enforce_order     = 0,
    .name              = "MTFrame (mtframe)",
    .settings          = NULL,
    .init              = mt_frame_init,
    .work              = mt_frame_work,
    .close             = mt_frame_close,
    .settings_template = mt_frame_template,
};

static int mt_frame_init(hb_filter_object_t * filter,
                         hb_filter_init_t   * init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("mt_frame: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;

    pv->sub_filter = filter->sub_filter;
    pv->sub_filter->init(pv->sub_filter, init);

    pv->thread_count = hb_get_cpu_count();
    pv->buf = calloc(pv->thread_count, sizeof(hb_buffer_t *));
    if (pv->buf == NULL)
    {
        goto fail;
    }

    pv->thread_data = malloc(pv->thread_count * sizeof(mt_frame_thread_arg_t *));
    if (pv->thread_data == NULL)
    {
        goto fail;
    }
    if (taskset_init(&pv->taskset, "mt_frame_filter", pv->thread_count,
                     sizeof(mt_frame_thread_arg_t), mt_frame_filter_work) == 0)
    {
        hb_error("MTFrame could not initialize taskset");
        goto fail;
    }

    for (int ii = 0; ii < pv->thread_count; ii++)
    {
        pv->thread_data[ii] = taskset_thread_args(&pv->taskset, ii);
        if (pv->thread_data[ii] == NULL)
        {
            hb_error("MTFrame could not create thread args");
            goto fail;
        }

        pv->thread_data[ii]->pv = pv;
        pv->thread_data[ii]->arg.taskset = &pv->taskset;
        pv->thread_data[ii]->arg.segment = ii;
    }

    if (pv->sub_filter->init_thread != NULL)
    {
        if (pv->sub_filter->init_thread(pv->sub_filter, pv->thread_count) < 0)
        {
            goto fail;
        }
    }

    return 0;

fail:
    taskset_fini(&pv->taskset);
    free(pv->thread_data);
    free(pv);
    return -1;
}

static void mt_frame_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    pv->sub_filter->close(pv->sub_filter);
    taskset_fini(&pv->taskset);

    for (int f = 0; f < pv->frame_count; f++)
    {
        if (pv->buf[f] != NULL)
        {
            hb_buffer_close(&pv->buf[f]);
        }
    }

    free(pv->thread_data);
    free(pv->buf);
    free(pv);
    filter->private_data = NULL;
}

static void mt_frame_filter_work(void *thread_args_v)
{
    mt_frame_thread_arg_t *thread_data = thread_args_v;
    hb_filter_private_t *pv = thread_data->pv;
    int segment = thread_data->arg.segment;

    if (pv->sub_filter->work_thread != NULL)
    {
        pv->sub_filter->work_thread(pv->sub_filter,
                             &pv->buf[segment], &thread_data->out, segment);
    }
    else
    {
        pv->sub_filter->work(pv->sub_filter,
                             &pv->buf[segment], &thread_data->out);
    }
    if (pv->buf[segment] != NULL)
    {
        hb_buffer_close(&pv->buf[segment]);
    }
}

static hb_buffer_t * mt_frame_filter(hb_filter_private_t *pv)
{
    if (pv->frame_count < pv->thread_count)
    {
        return NULL;
    }

    taskset_cycle(&pv->taskset);
    pv->frame_count = 0;

    // Collect results from taskset
    hb_buffer_list_t list;
    hb_buffer_list_clear(&list);
    for (int t = 0; t < pv->thread_count; t++)
    {
        hb_buffer_list_append(&list, pv->thread_data[t]->out);
    }
    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * mt_frame_filter_flush(hb_filter_private_t *pv)
{
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    for (int f = 0; f < pv->frame_count; f++)
    {
        hb_buffer_t * out;
        pv->sub_filter->work(pv->sub_filter, &pv->buf[f], &out);

        if (pv->buf[f] != NULL)
        {
            hb_buffer_close(&pv->buf[f]);
        }
        hb_buffer_list_append(&list, out);
    }
    pv->frame_count = 0;
    return hb_buffer_list_clear(&list);
}

static int mt_frame_work(hb_filter_object_t  * filter,
                         hb_buffer_t        ** buf_in,
                         hb_buffer_t        ** buf_out )
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    *buf_in  = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        hb_buffer_list_t list;
        hb_buffer_t *buf;

        // Flush buffered frames
        buf = mt_frame_filter_flush(pv);
        hb_buffer_list_set(&list, buf);

        // And terminate the buffer list with a EOF buffer
        hb_buffer_list_append(&list, in);
        *buf_out = hb_buffer_list_clear(&list);

        return HB_FILTER_DONE;
    }

    pv->buf[pv->frame_count++] = in;
    *buf_out = mt_frame_filter(pv);

    return HB_FILTER_OK;
}
