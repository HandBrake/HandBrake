/* encca_aac.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "audio_remap.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

enum AAC_MODE { AAC_MODE_LC, AAC_MODE_HE };

int  encCoreAudioInitLC(hb_work_object_t*, hb_job_t*);
int  encCoreAudioInitHE(hb_work_object_t*, hb_job_t*);
int  encCoreAudioInit(hb_work_object_t*, hb_job_t*, enum AAC_MODE mode);
int  encCoreAudioWork(hb_work_object_t*, hb_buffer_t**, hb_buffer_t**);
void encCoreAudioClose(hb_work_object_t*);

hb_work_object_t hb_encca_aac =
{
    WORK_ENC_CA_AAC,
    "AAC encoder (Apple)",
    encCoreAudioInitLC,
    encCoreAudioWork,
    encCoreAudioClose
};

hb_work_object_t hb_encca_haac =
{
    WORK_ENC_CA_HAAC,
    "HE-AAC encoder (Apple)",
    encCoreAudioInitHE,
    encCoreAudioWork,
    encCoreAudioClose
};

struct hb_work_private_s
{
    uint8_t *buf;
    hb_job_t *job;
    hb_list_t *list;

    AudioConverterRef converter;
    unsigned long isamples, isamplesiz, omaxpacket, nchannels;
    uint64_t samples, ibytes;
    Float64 osamplerate;

    hb_audio_remap_t *remap;
};

#define MP4ESDescrTag                   0x03
#define MP4DecConfigDescrTag            0x04
#define MP4DecSpecificDescrTag          0x05

// based off of mov_mp4_read_descr_len from mov.c in ffmpeg's libavformat
static int readDescrLen(UInt8 **buffer)
{
    int len = 0;
    int count = 4;
    while (count--)
    {
        int c = *(*buffer)++;
        len = (len << 7) | (c & 0x7f);
        if (!(c & 0x80))
            break;
    }
    return len;
}

// based off of mov_mp4_read_descr from mov.c in ffmpeg's libavformat
static int readDescr(UInt8 **buffer, int *tag)
{
    *tag = *(*buffer)++;
    return readDescrLen(buffer);
}

// based off of mov_read_esds from mov.c in ffmpeg's libavformat
static long ReadESDSDescExt(void* descExt, UInt8 **buffer, UInt32 *size, int versionFlags)
{
    UInt8 *esds = (UInt8*)descExt;
    int tag, len;
    *size = 0;

    if (versionFlags)
        esds += 4; // version + flags
    readDescr(&esds, &tag);
    esds += 2;     // ID
    if (tag == MP4ESDescrTag)
        esds++;    // priority

    readDescr(&esds, &tag);
    if (tag == MP4DecConfigDescrTag)
    {
        esds++;    // object type id
        esds++;    // stream type
        esds += 3; // buffer size db
        esds += 4; // max bitrate
        esds += 4; // average bitrate

        len = readDescr(&esds, &tag);
        if (tag == MP4DecSpecificDescrTag)
        {
            *buffer = calloc(1, len + 8);
            if (*buffer)
            {
                memcpy(*buffer, esds, len);
                *size = len;
            }
        }
    }

    return noErr;
}

/***********************************************************************
 * hb_work_encCoreAudio_init switches
 ***********************************************************************
 *
 **********************************************************************/
int encCoreAudioInitLC(hb_work_object_t *w, hb_job_t *job)
{
    return encCoreAudioInit(w, job, AAC_MODE_LC);
}

int encCoreAudioInitHE(hb_work_object_t *w, hb_job_t *job)
{
    return encCoreAudioInit(w, job, AAC_MODE_HE);
}

/***********************************************************************
 * hb_work_encCoreAudio_init
 ***********************************************************************
 *
 **********************************************************************/
int encCoreAudioInit(hb_work_object_t *w, hb_job_t *job, enum AAC_MODE mode)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    hb_audio_t *audio = w->audio;
    AudioStreamBasicDescription input, output;
    UInt32 tmp, tmpsiz = sizeof(tmp);
    OSStatus err;

    w->private_data = pv;
    pv->job = job;

    // pass the number of channels used into the private work data
    pv->nchannels =
        hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);

    bzero(&input, sizeof(AudioStreamBasicDescription));
    input.mSampleRate = (Float64)audio->config.out.samplerate;
    input.mFormatID = kAudioFormatLinearPCM;
    input.mFormatFlags = (kLinearPCMFormatFlagIsFloat|kAudioFormatFlagsNativeEndian);
    input.mBytesPerPacket = 4 * pv->nchannels;
    input.mFramesPerPacket = 1;
    input.mBytesPerFrame = input.mBytesPerPacket * input.mFramesPerPacket;
    input.mChannelsPerFrame = pv->nchannels;
    input.mBitsPerChannel = 32;

    bzero(&output, sizeof(AudioStreamBasicDescription));
    switch (mode)
    {
        case AAC_MODE_HE:
            output.mFormatID = kAudioFormatMPEG4AAC_HE;
            break;
        case AAC_MODE_LC:
        default:
            output.mFormatID = kAudioFormatMPEG4AAC;
            break;
    }
    output.mSampleRate = (Float64)audio->config.out.samplerate;
    output.mChannelsPerFrame = pv->nchannels;
    // let CoreAudio decide the rest

    // initialise encoder
    err = AudioConverterNew(&input, &output, &pv->converter);
    if (err != noErr)
    {
        // Retry without the samplerate
        bzero(&output, sizeof(AudioStreamBasicDescription));
        switch (mode)
        {
            case AAC_MODE_HE:
                output.mFormatID = kAudioFormatMPEG4AAC_HE;
                break;
            case AAC_MODE_LC:
            default:
                output.mFormatID = kAudioFormatMPEG4AAC;
                break;
        }
        output.mChannelsPerFrame = pv->nchannels;

        err = AudioConverterNew(&input, &output, &pv->converter);

        if (err != noErr)
        {
            hb_log("Error creating an AudioConverter err=%"PRId64" output.mBytesPerFrame=%"PRIu64"",
                   (int64_t)err, (uint64_t)output.mBytesPerFrame);
            *job->done_error = HB_ERROR_UNKNOWN;
            *job->die = 1;
            return -1;
        }
    }

    // set encoder quality to maximum
    tmp = kAudioConverterQuality_Max;
    AudioConverterSetProperty(pv->converter, kAudioConverterCodecQuality,
                              sizeof(tmp), &tmp);

    if (audio->config.out.bitrate > 0)
    {
        // set encoder bitrate control mode to constrained variable
        tmp = kAudioCodecBitRateControlMode_VariableConstrained;
        AudioConverterSetProperty(pv->converter,
                                  kAudioCodecPropertyBitRateControlMode,
                                  sizeof(tmp), &tmp);

        // get available bitrates
        AudioValueRange *bitrates;
        ssize_t bitrateCounts;
        err = AudioConverterGetPropertyInfo(pv->converter,
                                            kAudioConverterApplicableEncodeBitRates,
                                            &tmpsiz, NULL);
        bitrates = malloc(tmpsiz);
        err = AudioConverterGetProperty(pv->converter,
                                        kAudioConverterApplicableEncodeBitRates,
                                        &tmpsiz, bitrates);
        bitrateCounts = tmpsiz / sizeof(AudioValueRange);

        // set bitrate
        tmp = audio->config.out.bitrate * 1000;
        if (tmp < bitrates[0].mMinimum)
            tmp = bitrates[0].mMinimum;
        if (tmp > bitrates[bitrateCounts-1].mMinimum)
            tmp = bitrates[bitrateCounts-1].mMinimum;
        free(bitrates);
        if (tmp != audio->config.out.bitrate * 1000)
        {
            hb_log("encCoreAudioInit: sanitizing track %d audio bitrate %d to %"PRIu32"",
                   audio->config.out.track, audio->config.out.bitrate, tmp / 1000);
        }
        AudioConverterSetProperty(pv->converter,
                                  kAudioConverterEncodeBitRate,
                                  sizeof(tmp), &tmp);
    }
    else if (audio->config.out.quality >= 0)
    {
        if (mode != AAC_MODE_LC)
        {
            hb_error("encCoreAudioInit: internal error, VBR set but not applicable");
            return 1;
        }
        // set encoder bitrate control mode to variable
        tmp = kAudioCodecBitRateControlMode_Variable;
        AudioConverterSetProperty(pv->converter,
                                  kAudioCodecPropertyBitRateControlMode,
                                  sizeof(tmp), &tmp);

        // set quality
        tmp = audio->config.out.quality;
        AudioConverterSetProperty(pv->converter,
                                  kAudioCodecPropertySoundQualityForVBR,
                                  sizeof(tmp), &tmp);
    }
    else
    {
        hb_error("encCoreAudioInit: internal error, bitrate/quality not set");
        return 1;
    }

    // get real input
    tmpsiz = sizeof(input);
    AudioConverterGetProperty(pv->converter,
                              kAudioConverterCurrentInputStreamDescription,
                              &tmpsiz, &input);
    // get real output
    tmpsiz = sizeof(output);
    AudioConverterGetProperty(pv->converter,
                              kAudioConverterCurrentOutputStreamDescription,
                              &tmpsiz, &output);

    // set sizes
    pv->isamplesiz  = input.mBytesPerPacket;
    pv->isamples    = output.mFramesPerPacket;
    pv->osamplerate = output.mSampleRate;
    audio->config.out.samples_per_frame = pv->isamples;

    // channel remapping
    pv->remap = hb_audio_remap_init(AV_SAMPLE_FMT_FLT, &hb_aac_chan_map,
                                    audio->config.in.channel_map);
    if (pv->remap == NULL)
    {
        hb_error("encCoreAudioInit: hb_audio_remap_init() failed");
    }
    uint64_t layout = hb_ff_mixdown_xlat(audio->config.out.mixdown, NULL);
    hb_audio_remap_set_channel_layout(pv->remap, layout);

    // get maximum output size
    AudioConverterGetProperty(pv->converter,
                              kAudioConverterPropertyMaximumOutputPacketSize,
                              &tmpsiz, &tmp);
    pv->omaxpacket = tmp;

    // get magic cookie (elementary stream descriptor)
    tmp = HB_CONFIG_MAX_SIZE;
    AudioConverterGetProperty(pv->converter,
                              kAudioConverterCompressionMagicCookie,
                              &tmp, w->config->extradata.bytes);
    // CoreAudio returns a complete ESDS, but we only need
    // the DecoderSpecific info.
    UInt8* buffer = NULL;
    ReadESDSDescExt(w->config->extradata.bytes, &buffer, &tmpsiz, 0);
    w->config->extradata.length = tmpsiz;
    memmove(w->config->extradata.bytes, buffer, w->config->extradata.length);
    free(buffer);

    pv->list = hb_list_init();
    pv->buf = NULL;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encCoreAudioClose(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;

    if (pv != NULL)
    {
        if (pv->converter)
        {
            AudioConverterDispose(pv->converter);
        }
        if (pv->buf != NULL)
        {
            free(pv->buf);
        }
        if (pv->remap != NULL)
        {
            hb_audio_remap_free(pv->remap);
        }
        hb_list_empty(&pv->list);
        free(pv);
        w->private_data = NULL;
    }
}

/* Called whenever necessary by AudioConverterFillComplexBuffer */
static OSStatus inInputDataProc(AudioConverterRef converter, UInt32 *npackets,
                                AudioBufferList *buffers,
                                AudioStreamPacketDescription **ignored,
                                void *userdata)
{
    hb_work_private_t *pv = userdata;

    if (!pv->ibytes)
    {
        *npackets = 0;
        return 1;
    }

    if (pv->buf != NULL)
    {
        free(pv->buf);
    }

    buffers->mBuffers[0].mDataByteSize = MIN(pv->ibytes,
                                             pv->isamplesiz * *npackets);
    pv->buf = calloc(1, buffers->mBuffers[0].mDataByteSize);
    buffers->mBuffers[0].mData = pv->buf;

    if (hb_list_bytes(pv->list) >= buffers->mBuffers[0].mDataByteSize)
    {
        hb_list_getbytes(pv->list, buffers->mBuffers[0].mData,
                         buffers->mBuffers[0].mDataByteSize, NULL, NULL);
    }
    else
    {
        *npackets = 0;
        return 1;
    }

    *npackets = buffers->mBuffers[0].mDataByteSize / pv->isamplesiz;
    pv->ibytes -= buffers->mBuffers[0].mDataByteSize;

    hb_audio_remap(pv->remap, (uint8_t**)(&buffers->mBuffers[0].mData),
                   (int)(*npackets));

    return noErr;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t* Encode(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;
    UInt32 npackets = 1;

    /* check if we need more data */
    if ((pv->ibytes = hb_list_bytes(pv->list)) < pv->isamples * pv->isamplesiz)
    {
        return NULL;
    }

    hb_buffer_t *obuf;
    AudioStreamPacketDescription odesc = { 0 };
    AudioBufferList obuflist =
    {
        .mNumberBuffers = 1,
        .mBuffers = { { .mNumberChannels = pv->nchannels } },
    };

    obuf = hb_buffer_init(pv->omaxpacket);
    obuflist.mBuffers[0].mDataByteSize = obuf->size;
    obuflist.mBuffers[0].mData = obuf->data;

    OSStatus err = AudioConverterFillComplexBuffer(pv->converter,
                                                   inInputDataProc, pv,
                                                   &npackets, &obuflist, &odesc);

    if (err != noErr && err != 1)
    {
        hb_log("encCoreAudio: unexpected error in AudioConverterFillComplexBuffer()");
    }
    // only drop the output buffer if it's actually empty
    if (!npackets || odesc.mDataByteSize <= 0)
    {
        hb_log("encCoreAudio: 0 packets returned");
        return NULL;
    }

    obuf->size        = odesc.mDataByteSize;
    obuf->s.start     = 90000LL * pv->samples / pv->osamplerate;
    pv->samples      += pv->isamples;
    obuf->s.stop      = 90000LL * pv->samples / pv->osamplerate;
    obuf->s.duration  = (double)90000 * pv->isamples / pv->osamplerate;
    obuf->s.type      = AUDIO_BUF;
    obuf->s.frametype = HB_FRAME_AUDIO;

    return obuf;
}

static hb_buffer_t* Flush(hb_work_object_t *w, hb_buffer_t *bufin)
{
    hb_work_private_t *pv = w->private_data;

    // pad whatever data we have out to four input frames.
    int nbytes = hb_list_bytes(pv->list);
    int pad = pv->isamples * pv->isamplesiz - nbytes;
    if (pad > 0)
    {
        hb_buffer_t *tmp = hb_buffer_init(pad);
        memset(tmp->data, 0, pad);
        hb_list_add(pv->list, tmp);
    }

    hb_buffer_t *bufout = NULL, *buf = NULL;
    while (hb_list_bytes(pv->list) >= pv->isamples * pv->isamplesiz)
    {
        hb_buffer_t *b = Encode(w);
        if (b != NULL)
        {
            if (bufout == NULL)
            {
                bufout = b;
            }
            else
            {
                buf->next = b;
            }
            buf = b;
        }
    }
    // add the eof marker to the end of our buf chain
    if (buf != NULL)
    {
        buf->next = bufin;
    }
    else
    {
        bufout = bufin;
    }
    return bufout;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encCoreAudioWork(hb_work_object_t *w, hb_buffer_t **buf_in,
                     hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t *buf;

    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF on input. Finish encoding what we have buffered then send
        // it & the eof downstream.
        *buf_out = Flush(w, in);
        return HB_WORK_DONE;
    }

    hb_list_add(pv->list, in);

    *buf_out = buf = Encode(w);

    while (buf != NULL)
    {
        buf->next = Encode(w);
        buf       = buf->next;
    }

    return HB_WORK_OK;
}
