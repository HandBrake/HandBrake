/* enctx3gsub.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/extradata.h"
#include "handbrake/ssautil.h"

struct hb_work_private_s
{
    hb_job_t       *job;
    hb_subtitle_t  *subtitle;

    hb_tx3g_style_context_t *tx3g;
};

static int Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));

    if (pv == NULL)
    {
        hb_error("enctx3gsubInit: calloc private data failed");
        return 1;
    }

    w->private_data = pv;

    pv->job      = job;
    pv->subtitle = w->subtitle;
    pv->tx3g = hb_tx3g_style_init(job->height,
                                  pv->subtitle->extradata->bytes,
                                  pv->subtitle->extradata->size);

    if (pv->tx3g == NULL)
    {
        hb_error("enctx3gsubInit: hb_tx3g_style_init failed");
        goto fail;
    }

    // Build codec extradata for tx3g
    uint8_t properties[] = {
        0x00, 0x00, 0x00, 0x00,     // Display Flags
        0x01,                       // Horiz. Justification
        0xff,                       // Vert. Justification
        0x00, 0x00, 0x00, 0xff,     // Bg color
        0x00, 0x00, 0x00, 0x00,     // Default text box
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,     // Reserved
        0x00, 0x01,                 // Font ID
        0x00,                       // Font face
        0x18,                       // Font size
        0xff, 0xff, 0xff, 0xff,     // Fg color
        // Font table:
        0x00, 0x00, 0x00, 0x12,     // Font table size
        'f','t','a','b',            // Tag
        0x00, 0x01,                 // Count
        0x00, 0x01,                 // Font ID
        0x05,                       // Font name length
        'A','r','i','a','l'         // Font name
    };

    int width, height, font_size;
    width     = job->width * job->par.num / job->par.den;
    font_size = 0.05 * job->height;
    if (font_size < 12)
    {
        font_size = 12;
    }
    else if (font_size > 255)
    {
        font_size = 255;
    }
    properties[25] = font_size;
    height = 3 * font_size;
    properties[14] = height >> 8;
    properties[15] = height & 0xff;
    properties[16] = width >> 8;
    properties[17] = width & 0xff;

    pv->subtitle->width  = width;
    pv->subtitle->height = height;

    int ret = hb_set_extradata(&pv->subtitle->extradata,
                               properties,
                               sizeof(properties));
    if (ret != 0)
    {
        hb_error("enctx3gsubInit: set subtitle extradata failed");
        goto fail;
    }

    return 0;

fail:
    return 1;
}

static void Close(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;
    if (pv == NULL)
    {
        return;
    }
    hb_tx3g_style_close(&pv->tx3g);
    free(pv);
    w->private_data = NULL;
}

static int Work(hb_work_object_t *w,
                hb_buffer_t **buf_in,
                hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    hb_buffer_t *out = NULL;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF on input stream - send it downstream & say that we're done
        *buf_in = NULL;
        *buf_out = in;
        return HB_WORK_DONE;
    }

    uint8_t  *styleatom;
    uint16_t  stylesize = 0;
    uint8_t  *buffer;
    uint16_t  buffersize = 0;

    // Copy the subtitle into buffer stripping markup and
    // creating style atoms for them.
    hb_muxmp4_process_subtitle_style(pv->tx3g, in->data, &buffer,
                                     &styleatom, &stylesize);

    if (buffer != NULL)
    {
        buffersize = strlen((char *)buffer);
        if (styleatom == NULL)
        {
            stylesize = 0;
        }
        out = hb_buffer_init(2 + buffersize + stylesize);
        out->s = in->s;

        // Write the subtitle sample
        memcpy(out->data + 2, buffer, buffersize);
        memcpy(out->data + 2 + buffersize, styleatom, stylesize);
        out->data[0] = (buffersize >> 8) & 0xff;
        out->data[1] = buffersize & 0xff;
    }
    free(buffer);
    free(styleatom);

    *buf_out = out;

    return HB_WORK_OK;
}

static int Info(hb_work_object_t *w, hb_work_info_t *info)
{
    memset(info, 0, sizeof(*info));

    // Indicate no info is returned
    return 0;
}

static int BSInfo(hb_work_object_t *w, const hb_buffer_t *buf,
                  hb_work_info_t *info)
{
    memset(info, 0, sizeof(*info));

    // Indicate no info is returned
    return 0;
}

static void Flush(hb_work_object_t *w) {}

hb_work_object_t hb_enctx3gsub =
{
    .id     = WORK_ENCTX3GSUB,
    .name   = "TX3G encoder",
    .init   = Init,
    .work   = Work,
    .close  = Close,
    .info   = Info,
    .bsinfo = BSInfo,
    .flush  = Flush,
};
