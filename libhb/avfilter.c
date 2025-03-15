/* avfilter.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/hbavfilter.h"
#include "handbrake/avfilter_priv.h"

#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
#include "handbrake/qsv_common.h"
#endif

static int  avfilter_init(hb_filter_object_t * filter, hb_filter_init_t * init);
static int  avfilter_post_init( hb_filter_object_t * filter, hb_job_t * job );
static void avfilter_close( hb_filter_object_t * filter );
static int  avfilter_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );
static hb_filter_info_t * avfilter_info( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_avfilter =
{
    .id            = HB_FILTER_AVFILTER,
    .enforce_order = 0,
    .name          = "AVFilter",
    .settings      = NULL,
    .init          = avfilter_init,
    .post_init     = avfilter_post_init,
    .work          = avfilter_work,
    .close         = avfilter_close,
    .info          = avfilter_info,
};

int  hb_avfilter_null_work( hb_filter_object_t * filter,
                            hb_buffer_t ** buf_in, hb_buffer_t ** buf_out )
{
    hb_log("hb_avfilter_null_work: It is an error to call this function.");
    return HB_WORK_DONE;
}

static int avfilter_init( hb_filter_object_t * filter, hb_filter_init_t * init )
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;
    pv->initialized = 1;

    pv->graph = hb_avfilter_graph_init(filter->settings, init);
    if (pv->graph == NULL)
    {
        goto fail;
    }

    // Retrieve the parameters of the output filter
    hb_avfilter_graph_update_init(pv->graph, init);
    pv->output = *init;

    hb_buffer_list_clear(&pv->list);

    return 0;

fail:
    hb_avfilter_graph_close(&pv->graph);
    free(pv);

    return 1;
}

// avfilter_post_init is used to finalize filters that are aliases
// to avfilter (pad, rotate, cropscale, deinterlace, and colorspace).
// These filters have their own init function that gets called during
// the initial filter init pass.  Then in post_init, the AVFilterGraph
// that does the real work is initialized.
static int avfilter_post_init( hb_filter_object_t * filter, hb_job_t * job )
{
    hb_filter_private_t * pv = filter->private_data;

    if (pv == NULL)
    {
        return 1;
    }
    if (pv->initialized)
    {
        return 0;
    }

    pv->graph = hb_avfilter_graph_init(filter->settings, &pv->input);
    if (pv->graph == NULL)
    {
        goto fail;
    }

    // Retrieve the parameters of the output filter
    pv->output = pv->input;
    hb_avfilter_graph_update_init(pv->graph, &pv->output);

    hb_buffer_list_clear(&pv->list);

    return 0;

fail:
    hb_avfilter_graph_close(&pv->graph);
    free(pv);

    return 1;
}

static hb_filter_info_t * avfilter_info(hb_filter_object_t * filter)
{
    hb_filter_private_t * pv = filter->private_data;
    hb_filter_info_t    * info;

    if (global_verbosity_level < 2)
    {
        // Only show this for log levels 2 and above
        return NULL;
    }
    if (pv == NULL)
    {
        return NULL;
    }

    info = calloc(1, sizeof(hb_filter_info_t));
    if (info == NULL)
    {
        hb_error("avfilter_info: allocation failure");
        return NULL;
    }
    info->output = pv->output;
    info->human_readable_desc = malloc(1024);
    if (info->human_readable_desc == NULL)
    {
        free(info);
        hb_error("avfilter_info: allocation failure");
        return NULL;
    }
    info->human_readable_desc[0] = 0;

    char       * dst   = info->human_readable_desc;
    const char * start = hb_avfilter_graph_settings(pv->graph);
    while (start != NULL && *start != 0)
    {
        // Find end of a filter
        char * comma = strchr(start, ',');
        char * quote = strchr(start, '\'');
        if (comma != NULL && quote != NULL && quote < comma)
        {
            // Find end of quote
            //quote = strchr(quote+1, '\'');
            comma = strchr(start, ',');
        }
        // pretty print line
        int name = 1;
        while (*start != 0 && (comma == NULL || start < comma))
        {
            switch (*start)
            {
                case '=':
                    if (name)
                    {
                        *dst++ = ':';
                        *dst++ = ' ';
                        name = 0;
                    }
                    else
                    {
                        *dst++ = '=';
                    }
                    break;

                case ':':
                    *dst++ = ',';
                    *dst++ = ' ';
                    break;

                case '\'':
                case ' ':
                    break;

                default:
                    *dst++ = *start;

            }
            start++;
        }
        if (*start != 0)
        {
            *dst++ = '\n';
            start++;
        }
    }
    *dst = 0;
    return info;
}

static void avfilter_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    if (pv == NULL)
    {
        // Already closed
        return;
    }

    hb_buffer_list_close(&pv->list);
    hb_avfilter_graph_close(&pv->graph);
    free(pv);
    filter->private_data = NULL;
}

void hb_avfilter_alias_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    if (pv == NULL)
    {
        // Already closed
        return;
    }

    hb_buffer_list_close(&pv->list);
    hb_value_free(&pv->avfilters);
    free(pv);
    filter->private_data = NULL;
}

static hb_buffer_t* filterFrame( hb_filter_private_t * pv, hb_buffer_t ** buf_in )
{
    hb_buffer_list_t   list;
    hb_buffer_t      * buf = NULL, * next = NULL;

    hb_avfilter_add_buf(pv->graph, buf_in);
    buf = hb_avfilter_get_buf(pv->graph);

    while (buf != NULL)
    {
        hb_buffer_list_append(&pv->list, buf);
        buf = hb_avfilter_get_buf(pv->graph);
    }
    // Delay one frame so we can set the stop time of the output buffer
    hb_buffer_list_clear(&list);
    while (hb_buffer_list_count(&pv->list) > 1)
    {
        buf  = hb_buffer_list_rem_head(&pv->list);
        next = hb_buffer_list_head(&pv->list);

        buf->s.stop = next->s.start;
        buf->s.duration = buf->s.stop - buf->s.start;
        hb_buffer_list_append(&list, buf);
    }

    return hb_buffer_list_head(&list);
}

static int avfilter_work( hb_filter_object_t * filter,
                          hb_buffer_t ** buf_in, hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        hb_buffer_t * out  = filterFrame(pv, NULL);
        hb_buffer_t * last = hb_buffer_list_tail(&pv->list);
        if (last != NULL && last->s.start != AV_NOPTS_VALUE)
        {
            last->s.stop = last->s.start + last->s.duration;
        }
        hb_buffer_list_prepend(&pv->list, out);
        hb_buffer_list_append(&pv->list, in);
        *buf_out = hb_buffer_list_clear(&pv->list);
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    *buf_out = filterFrame(pv, buf_in);

    return HB_FILTER_OK;
}
