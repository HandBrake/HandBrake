/* This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

int     encCoreAudioInit( hb_work_object_t *, hb_job_t * );
int     encCoreAudioWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void    encCoreAudioClose( hb_work_object_t * );

hb_work_object_t hb_encca_aac =
{
    WORK_ENC_CA_AAC,
    "AAC encoder (Apple)",
    encCoreAudioInit,
    encCoreAudioWork,
    encCoreAudioClose
};

struct hb_work_private_s
{
    hb_job_t *job;
    
    AudioConverterRef converter;
    uint8_t  *obuf;
    uint8_t  *buf;
    hb_list_t *list;
    unsigned long isamples, isamplesiz, omaxpacket, nchannels;
    uint64_t pts, ibytes;
    Float64 osamplerate;
};

#define MP4ESDescrTag                   0x03
#define MP4DecConfigDescrTag            0x04
#define MP4DecSpecificDescrTag          0x05

// based off of mov_mp4_read_descr_len from mov.c in ffmpeg's libavformat
static int readDescrLen(UInt8 **buffer)
{
	int len = 0;
	int count = 4;
	while (count--) {
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
	UInt8 *esds = (UInt8 *) descExt;
	int tag, len;
	*size = 0;

    if (versionFlags)
        esds += 4;		// version + flags
	readDescr(&esds, &tag);
	esds += 2;		// ID
	if (tag == MP4ESDescrTag)
		esds++;		// priority

	readDescr(&esds, &tag);
	if (tag == MP4DecConfigDescrTag) {
		esds++;		// object type id
		esds++;		// stream type
		esds += 3;	// buffer size db
		esds += 4;	// max bitrate
		esds += 4;	// average bitrate

		len = readDescr(&esds, &tag);
		if (tag == MP4DecSpecificDescrTag) {
			*buffer = calloc(1, len + 8);
			if (*buffer) {
				memcpy(*buffer, esds, len);
				*size = len;
			}
		}
	}

	return noErr;
}

/***********************************************************************
 * hb_work_encCoreAudio_init
 ***********************************************************************
 *
 **********************************************************************/
int encCoreAudioInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;
    AudioStreamBasicDescription input, output;
    UInt32 tmp, tmpsiz = sizeof( tmp );
    OSStatus err;

    w->private_data = pv;
    pv->job = job;

    // pass the number of channels used into the private work data
    pv->nchannels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT( audio->config.out.mixdown );

    bzero( &input, sizeof( AudioStreamBasicDescription ) );
    input.mSampleRate = ( Float64 ) audio->config.out.samplerate;
    input.mFormatID = kAudioFormatLinearPCM;
    input.mFormatFlags = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagsNativeEndian;
    input.mBytesPerPacket = 4 * pv->nchannels;
    input.mFramesPerPacket = 1;
    input.mBytesPerFrame = input.mBytesPerPacket * input.mFramesPerPacket;
    input.mChannelsPerFrame = pv->nchannels;
    input.mBitsPerChannel = 32;

    bzero( &output, sizeof( AudioStreamBasicDescription ) );
    output.mFormatID = kAudioFormatMPEG4AAC;
    output.mSampleRate = ( Float64 ) audio->config.out.samplerate;
    output.mChannelsPerFrame = pv->nchannels;
    // let CoreAudio decide the rest...

    // initialise encoder
    err = AudioConverterNew( &input, &output, &pv->converter );
    if( err != noErr)
    {
        // Retry without the samplerate
        bzero( &output, sizeof( AudioStreamBasicDescription ) );
        output.mFormatID = kAudioFormatMPEG4AAC;
        output.mChannelsPerFrame = pv->nchannels;

        err = AudioConverterNew( &input, &output, &pv->converter );

        if( err != noErr)
        {
            hb_log( "Error creating an AudioConverter err=%"PRId64" %"PRIu64, (int64_t)err, (uint64_t)output.mBytesPerFrame );
            *job->die = 1;
            return 0;
        }
    }

    if( audio->config.out.mixdown == HB_AMIXDOWN_6CH && audio->config.in.codec == HB_ACODEC_AC3 )
    {
        SInt32 channelMap[6] = { 2, 1, 3, 4, 5, 0 };
        AudioConverterSetProperty( pv->converter, kAudioConverterChannelMap,
                                   sizeof( channelMap ), channelMap );
    }

    // set encoder quality to maximum
    tmp = kAudioConverterQuality_Max;
    AudioConverterSetProperty( pv->converter, kAudioConverterCodecQuality,
                               sizeof( tmp ), &tmp );

    // set encoder bitrate control mode to constrained variable
    tmp = kAudioCodecBitRateControlMode_VariableConstrained;
    AudioConverterSetProperty( pv->converter, kAudioCodecPropertyBitRateControlMode,
                              sizeof( tmp ), &tmp );

    // get available bitrates
    AudioValueRange *bitrates;
    ssize_t bitrateCounts;
    err = AudioConverterGetPropertyInfo( pv->converter, kAudioConverterApplicableEncodeBitRates,
                                         &tmpsiz, NULL);
    bitrates = malloc( tmpsiz );
    err = AudioConverterGetProperty( pv->converter, kAudioConverterApplicableEncodeBitRates,
                                     &tmpsiz, bitrates);
    bitrateCounts = tmpsiz / sizeof( AudioValueRange );

    // set bitrate
    tmp = audio->config.out.bitrate * 1000;
    if( tmp < bitrates[0].mMinimum )
        tmp = bitrates[0].mMinimum;
    if( tmp > bitrates[bitrateCounts-1].mMinimum )
        tmp = bitrates[bitrateCounts-1].mMinimum;
    free( bitrates );
    AudioConverterSetProperty( pv->converter, kAudioConverterEncodeBitRate,
                              sizeof( tmp ), &tmp );

    // get real input
    tmpsiz = sizeof( input );
    AudioConverterGetProperty( pv->converter,
                               kAudioConverterCurrentInputStreamDescription,
                               &tmpsiz, &input );
    // get real output
    tmpsiz = sizeof( output );
    AudioConverterGetProperty( pv->converter,
                               kAudioConverterCurrentOutputStreamDescription,
                               &tmpsiz, &output );

    // set sizes
    pv->isamplesiz  = input.mBytesPerPacket;
    pv->isamples    = output.mFramesPerPacket;
    pv->osamplerate = output.mSampleRate;

    // get maximum output size
    AudioConverterGetProperty( pv->converter,
                               kAudioConverterPropertyMaximumOutputPacketSize,
                               &tmpsiz, &tmp );
    pv->omaxpacket = tmp;

    // get magic cookie (elementary stream descriptor)
    tmp = HB_CONFIG_MAX_SIZE;
    AudioConverterGetProperty( pv->converter,
                               kAudioConverterCompressionMagicCookie,
                               &tmp, w->config->aac.bytes );
    // CoreAudio returns a complete ESDS, but we only need
    // the DecoderSpecific info.
    UInt8* buffer = NULL;
    ReadESDSDescExt(w->config->aac.bytes, &buffer, &tmpsiz, 0);
    w->config->aac.length = tmpsiz;
    memmove( w->config->aac.bytes, buffer,
             w->config->aac.length );

    pv->list = hb_list_init();
    pv->buf = NULL;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encCoreAudioClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if( pv->converter )
    {
        AudioConverterDispose( pv->converter );
        hb_list_empty( &pv->list );
        free( pv->obuf );
        free( pv->buf );
        free( pv );
        w->private_data = NULL;
    }
}

/* Called whenever necessary by AudioConverterFillComplexBuffer */
static OSStatus inInputDataProc( AudioConverterRef converter, UInt32 *npackets,
                          AudioBufferList *buffers,
                          AudioStreamPacketDescription** ignored,
                          void *userdata )
{
    hb_work_private_t *pv = userdata;
    pv->ibytes = hb_list_bytes( pv->list );

    if( pv->ibytes == 0 ) {
        *npackets = 0;
        return noErr;
    }

    if( pv->buf != NULL )
        free( pv->buf );

    uint64_t pts, pos;
    pv->ibytes = buffers->mBuffers[0].mDataByteSize = MIN( *npackets * pv->isamplesiz, pv->ibytes );
    buffers->mBuffers[0].mData = pv->buf = malloc( buffers->mBuffers[0].mDataByteSize );

    hb_list_getbytes( pv->list, buffers->mBuffers[0].mData,
                      buffers->mBuffers[0].mDataByteSize, &pts, &pos );

    *npackets = buffers->mBuffers[0].mDataByteSize / pv->isamplesiz;

    /* transform data from [-32768,32767] to [-1.0,1.0] */
    float *fdata = buffers->mBuffers[0].mData;
    int i;

    for( i = 0; i < *npackets * pv->nchannels; i++ )
        fdata[i] = fdata[i] / 32768.f;

    return noErr;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    UInt32 npackets = 1;

    /* check if we need more data */
    if( hb_list_bytes( pv->list ) < pv->isamples * pv->isamplesiz )
        return NULL;

    hb_buffer_t * obuf;
    AudioStreamPacketDescription odesc = { 0 };
    AudioBufferList obuflist = { .mNumberBuffers = 1,
                                 .mBuffers = { { .mNumberChannels = pv->nchannels } },
                               };

    obuf = hb_buffer_init( pv->omaxpacket );
    obuflist.mBuffers[0].mDataByteSize = obuf->size;
    obuflist.mBuffers[0].mData = obuf->data;

    AudioConverterFillComplexBuffer( pv->converter, inInputDataProc, pv, 
                                     &npackets, &obuflist, &odesc );

    if( odesc.mDataByteSize == 0 )
        return NULL;

    obuf->start = pv->pts;
    pv->pts += 90000LL * pv->isamples / pv->osamplerate;
    obuf->stop  = pv->pts;
    obuf->size  = odesc.mDataByteSize;
    obuf->frametype = HB_FRAME_AUDIO;

    return obuf;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encCoreAudioWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if( (*buf_in)->size <= 0 )
    {
        // EOF on input - send it downstream & say we're done
        *buf_out = *buf_in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    *buf_out = buf = Encode( w );

    while( buf )
    {
        buf->next = Encode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}
