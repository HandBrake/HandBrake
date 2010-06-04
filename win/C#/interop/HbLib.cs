// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HbLib.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the NativeConstants type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using System;
    using System.Runtime.InteropServices;

    internal partial class NativeConstants
    {
        public const int HB_ACODEC_MASK =   0x00FF00;
        public const int HB_ACODEC_FAAC =   0x000100;
        public const int HB_ACODEC_LAME =   0x000200;
        public const int HB_ACODEC_VORBIS = 0x000400;
        public const int HB_ACODEC_AC3 =    0x000800;
        public const int HB_ACODEC_MPGA =   0x001000;
        public const int HB_ACODEC_LPCM =   0x002000;
        public const int HB_ACODEC_DCA =    0x004000;
        public const int HB_ACODEC_FFMPEG = 0x008000;
        public const int HB_ACODEC_CA_AAC = 0x010000;

        public const int HB_AMIXDOWN_DCA_FORMAT_MASK =              0x00FFF000;
        public const int HB_AMIXDOWN_A52_FORMAT_MASK =              0x00000FF0;
        public const int HB_AMIXDOWN_DISCRETE_CHANNEL_COUNT_MASK =  0x0000000F;
        public const int HB_AMIXDOWN_MONO =                         0x01000001;
        public const int HB_AMIXDOWN_STEREO =                       0x02002022;
        public const int HB_AMIXDOWN_DOLBY =                        0x042070A2;
        public const int HB_AMIXDOWN_DOLBYPLII =                    0x084094A2;
        public const int HB_AMIXDOWN_6CH =                          0x10089176;

        public const int HB_VCODEC_MASK =   0x0000FF;
        public const int HB_VCODEC_FFMPEG = 0x000001;
        public const int HB_VCODEC_X264 =   0x000002;
        public const int HB_VCODEC_THEORA = 0x000004;

        public const int HB_MUX_MASK =  0xFF0000;
        public const int HB_MUX_MP4 =   0x010000;
        public const int HB_MUX_PSP =   0x020000;
        public const int HB_MUX_AVI =   0x040000;
        public const int HB_MUX_OGM =   0x080000;
        public const int HB_MUX_IPOD =  0x100000;
        public const int HB_MUX_MKV =   0x200000;

        public const int HBTF_NO_IDR = 1 << 0;

        public const int HB_STATE_IDLE = 1;
        public const int HB_STATE_SCANNING = 2;
        public const int HB_STATE_SCANDONE = 4;
        public const int HB_STATE_WORKING = 8;
        public const int HB_STATE_PAUSED = 16;
        public const int HB_STATE_WORKDONE = 32;
        public const int HB_STATE_MUXING = 64;

        public const int HB_ERROR_NONE = 0;
        public const int HB_ERROR_CANCELED = 1;
        public const int HB_ERROR_UNKNOWN = 2;

        public const int AUDIO_F_DOLBY = 1 << 31;

        public const int HB_FRAME_IDR =     0x01;
        public const int HB_FRAME_I =       0x02;
        public const int HB_FRAME_AUDIO =   0x04;
        public const int HB_FRAME_P =       0x10;
        public const int HB_FRAME_B =       0x20;
        public const int HB_FRAME_BREF =    0x40;
        public const int HB_FRAME_KEY =     0x0F;
        public const int HB_FRAME_REF =     0xF0;

        public const int HB_CONFIG_MAX_SIZE = 8192;

        public const int HB_FILTER_DETELECINE = 1;
        public const int HB_FILTER_DEINTERLACE = 2;
        public const int HB_FILTER_DEBLOCK = 3;
        public const int HB_FILTER_DENOISE = 4;
        public const int HB_FILTER_DECOMB = 5;
        public const int HB_FILTER_ROTATE = 6;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_anamorphic_substruct
    {
        /// int
        public int mode;

        /// int
        public int itu_par;

        /// int
        public int par_width;

        /// int
        public int par_height;

        /// int
        public int dar_width;

        /// int
        public int dar_height;

        /// int
        public int keep_display_aspect;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_job_s
    {
        /// int
        public int sequence_id;

        /// hb_title_t*
        public IntPtr title;

        public int feature;

        /// int
        public int chapter_start;

        /// int
        public int chapter_end;

        /// int
        public int chapter_markers;

        /// int[4]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
        public int[] crop;

        /// int
        public int deinterlace;

        /// hb_list_t*
        public IntPtr filters;

        /// int
        public int width;

        /// int
        public int height;

        /// int
        public int keep_ratio;

        /// int
        public int grayscale;

        public hb_anamorphic_substruct anamorphic;

        public int modulus;

        /// int
        public int maxWidth;

        /// int
        public int maxHeight;

        /// int
        public int vcodec;

        /// float
        public float vquality;

        /// int
        public int vbitrate;

        /// int
        public int vrate;

        /// int
        public int vrate_base;

        /// int
        public int vfr;

        /// int
        public int cfr;

        /// int
        public int pass;

        /// int
        public int h264_13;

        /// int
        public int h264_level;

        /// char*
        //[MarshalAs(UnmanagedType.LPStr)]
        //public string x264opts;

        public IntPtr x264opts;

        /// int
        public int areBframes;

        /// int
        public int color_matrix;

        /// hb_list_t*
        public IntPtr list_audio;

        /// hb_list_t*
        public IntPtr list_subtitle;

        /// int
        public int mux;

        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string file;

        /// int
        public int largeFileSize;

        /// int
        public int mp4_optimize;

        /// int
        public int ipod_atom;

        /// int
        public int indepth_scan;

        /// hb_subtitle_config_t->hb_subtitle_config_s
        public hb_subtitle_config_s select_subtitle_config;

        /// int
        public int angle;

        public int frame_to_start;

        public long pts_to_start;

        /// int
        public int frame_to_stop;

        /// int64_t->int
        public long pts_to_stop;

        /// int
        public int start_at_preview;

        /// int
        public int seek_points;

        /// uint32_t->unsigned int
        public uint frames_to_skip;

        // Padding for the part of the struct we don't care about marshaling.
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 24644, ArraySubType = UnmanagedType.U1)]
        public byte[] padding;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_list_s
    {
        /// void**
        public IntPtr items;

        /// int
        public int items_alloc;

        /// int
        public int items_count;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_rate_s
    {
        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string @string;

        /// int
        public int rate;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_handle_s
    {
        public int id;

        /// int
        public int build;

        /// char[32]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string version;

        /// hb_thread_t*
        public IntPtr update_thread;

        /// int
        public int die;

        /// hb_thread_t*
        public IntPtr main_thread;

        /// int
        public int pid;

        /// hb_list_t*
        public IntPtr list_title;

        /// hb_thread_t*
        public IntPtr scan_thread;

        /// hb_list_t*
        public IntPtr jobs;

        /// hb_job_t*
        public IntPtr current_job;

        /// int
        public int job_count;

        /// int
        public int job_count_permanent;

        /// int
        public int work_die;

        /// int
        public int work_error;

        /// hb_thread_t*
        public IntPtr work_thread;

        /// int
        public int cpu_count;

        /// hb_lock_t*
        public IntPtr state_lock;

        /// hb_state_t->hb_state_s
        public hb_state_s state;

        /// int
        public int paused;

        /// hb_lock_t*
        public IntPtr pause_lock;

        /// int
        public int scanCount;

        /// hb_interjob_t*
        public IntPtr interjob;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_chapter_s
    {
        /// int
        public int index;

        /// int
        public int pgcn;

        /// int
        public int pgn;

        /// int
        public int cell_start;

        /// int
        public int cell_end;

        /// int
        public int block_start;

        /// int
        public int block_end;

        /// int
        public int block_count;

        /// int
        public int hours;

        /// int
        public int minutes;

        /// int
        public int seconds;

        /// uint64_t->unsigned int
        public ulong duration;

        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string title;
    }

    internal enum hb_subtitle_s_subtype
    {
        PICTURESUB,

        TEXTSUB,
    }

    internal enum hb_subtitle_s_subsource
    {
        VOBSUB,

        SRTSUB,

        CC608SUB,

        CC708SUB,

        UTF8SUB,

        TX3GSUB
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_subtitle_s
    {
        /// int
        public int id;

        /// int
        public int track;

        /// hb_subtitle_config_t->hb_subtitle_config_s
        public hb_subtitle_config_s config;

        /// hb_subtitle_s_subtype
        public hb_subtitle_s_subtype format;

        /// hb_subtitle_s_subsource
        public hb_subtitle_s_subsource source;

        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string lang;

        /// char[4]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
        public string iso639_2;

        /// uint8_t->unsigned char
        public byte type;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16, ArraySubType = UnmanagedType.U4)]
        public uint[] palette;

        public int width;

        public int height;

        /// int
        public int hits;

        /// int
        public int forced_hits;

        /// hb_fifo_t*
        public IntPtr fifo_in;

        /// hb_fifo_t*
        public IntPtr fifo_raw;

        /// hb_fifo_t*
        public IntPtr fifo_sync;

        /// hb_fifo_t*
        public IntPtr fifo_out;

        /// hb_mux_data_t*
        public IntPtr mux_data;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_metadata_s
    {
        /// char[255]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
        public string name;

        /// char[255]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
        public string artist;

        /// char[255]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
        public string composer;

        /// char[255]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
        public string release_date;

        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string comment;

        /// char[255]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
        public string album;

        /// char[255]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
        public string genre;

        /// uint32_t->unsigned int
        public uint coverart_size;

        /// uint8_t*
        public IntPtr coverart;
    }

    internal enum Anonymous_990d28ea_6cf3_4fbc_8143_4df9513e9550
    {
        HB_DVD_TYPE,

        HB_STREAM_TYPE,
    }

    internal enum Anonymous_618ebeca_0ad9_4a71_9a49_18e50ac2e9db
    {
        /// HB_MPEG2_PS_DEMUXER -> 0
        HB_MPEG2_PS_DEMUXER = 0,

        HB_MPEG2_TS_DEMUXER,

        HB_NULL_DEMUXER,
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_title_s
    {
        /// Anonymous_990d28ea_6cf3_4fbc_8143_4df9513e9550
        public Anonymous_990d28ea_6cf3_4fbc_8143_4df9513e9550 type;

        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string dvd;

        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string name;

        //public fixed byte dvd[1024];

        //public fixed byte name[1024];

        /// int
        public int index;

        /// int
        public int vts;

        /// int
        public int ttn;

        /// int
        public int cell_start;

        /// int
        public int cell_end;

        /// int
        public int block_start;

        /// int
        public int block_end;

        /// int
        public int block_count;

        /// int
        public int angle_count;

        /// int
        public int hours;

        /// int
        public int minutes;

        /// int
        public int seconds;

        /// uint64_t->unsigned int
        public ulong duration;

        /// double
        public double aspect;

        /// double
        public double container_aspect;

        /// int
        public int width;

        /// int
        public int height;

        /// int
        public int pixel_aspect_width;

        /// int
        public int pixel_aspect_height;

        /// int
        public int rate;

        /// int
        public int rate_base;

        /// int[4]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
        public int[] crop;

        //public fixed int crop[4];

        /// Anonymous_618ebeca_0ad9_4a71_9a49_18e50ac2e9db
        public Anonymous_618ebeca_0ad9_4a71_9a49_18e50ac2e9db demuxer;

        /// int
        public int detected_interlacing;

        /// int
        public int video_id;

        /// int
        public int video_codec;

        /// int
        public int video_codec_param;

        /// char*
        public IntPtr video_codec_name;

        /// int
        public int video_bitrate;

        /// char*
        public IntPtr container_name;

        /// int
        public int data_rate;

        /// hb_metadata_t*
        public IntPtr metadata;

        /// hb_list_t*
        public IntPtr list_chapter;

        /// hb_list_t*
        public IntPtr list_audio;

        /// hb_list_t*
        public IntPtr list_subtitle;

        /// hb_job_t*
        public IntPtr job;

        /// uint32_t->unsigned int
        public uint flags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_state_scanning_s
    {
        /// int
        public int title_cur;

        /// int
        public int title_count;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_state_working_s
    {
        /// float
        public float progress;

        /// int
        public int job_cur;

        /// int
        public int job_count;

        /// float
        public float rate_cur;

        /// float
        public float rate_avg;

        /// int
        public int hours;

        /// int
        public int minutes;

        /// int
        public int seconds;

        /// int
        public int sequence_id;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_state_workdone_s
    {
        /// int
        public int error;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_state_muxing_s
    {
        /// float
        public float progress;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct hb_state_param_u
    {
        [FieldOffset(0)]
        public hb_state_scanning_s scanning;

        [FieldOffset(0)]
        public hb_state_working_s working;

        [FieldOffset(0)]
        public hb_state_workdone_s workdone;

        [FieldOffset(0)]
        public hb_state_muxing_s muxing;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_state_s
    {

        /// int
        public int state;
        public hb_state_param_u param;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_audio_s
    {
        /// int
        public int id;

        /// hb_audio_config_t->hb_audio_config_s
        public hb_audio_config_s config;

        // Padding for the part of the struct we don't care about marshaling.
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 24600, ArraySubType = UnmanagedType.U1)]
        public byte[] padding;

        /// Anonymous_e6c7b779_b5a3_4e80_9fa8_13619d14f545
        //public Anonymous_e6c7b779_b5a3_4e80_9fa8_13619d14f545 priv;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_audio_config_s
    {
        public hb_audio_config_output_s output;
        public hb_audio_config_input_s input;

        /// Anonymous_a0a59d69_d9a4_4003_a198_f7c51511e31d
        public Anonymous_a0a59d69_d9a4_4003_a198_f7c51511e31d flags;

        public hb_audio_config_lang_s lang;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_audio_config_output_s
    {
        /// int
        public int track;

        /// uint32_t->unsigned int
        public uint codec;

        /// int
        public int samplerate;

        /// int
        public int bitrate;

        /// int
        public int mixdown;

        /// double
        public double dynamic_range_compression;

        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string name;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_audio_config_input_s
    {
        /// int
        public int track;

        /// uint32_t->unsigned int
        public uint codec;

        /// uint32_t->unsigned int
        public uint codec_param;

        /// uint32_t->unsigned int
        public uint version;

        /// uint32_t->unsigned int
        public uint mode;

        /// int
        public int samplerate;

        /// int
        public int bitrate;

        /// int
        public int channel_layout;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct Anonymous_a0a59d69_d9a4_4003_a198_f7c51511e31d
    {
        /// int
        [FieldOffset(0)]
        public int ac3;

        /// int
        [FieldOffset(0)]
        public int dca;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_audio_config_lang_s
    {
        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string description;

        /// char[1024]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string simple;

        /// char[4]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
        public string iso639_2;

        /// uint8_t->unsigned char
        public byte type;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_mixdown_s
    {
        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string human_readable_name;

        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string internal_name;

        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string short_name;

        /// int
        public int amixdown;
    }

    internal enum hb_subtitle_config_s_subdest
    {
        RENDERSUB,

        PASSTHRUSUB,
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct hb_subtitle_config_s
    {
        /// hb_subtitle_config_s_subdest
        public hb_subtitle_config_s_subdest dest;

        /// int
        public int force;

        /// int
        public int default_track;

        /// char[128]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string src_filename;

        /// char[40]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 40)]
        public string src_codeset;

        /// int64_t->int
        public long offset;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_fifo_s
    {
        /// hb_lock_t*
        public IntPtr @lock;

        /// uint32_t->unsigned int
        public uint capacity;

        /// uint32_t->unsigned int
        public uint size;

        /// uint32_t->unsigned int
        public uint buffer_size;

        /// hb_buffer_t*
        public IntPtr first;

        /// hb_buffer_t*
        public IntPtr last;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_lock_s
    {
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_buffer_s
    {
        /// int
        public int size;

        /// int
        public int alloc;

        /// uint8_t*
        [MarshalAs(UnmanagedType.LPStr)]
        public string data;

        /// int
        public int cur;

        /// int64_t->int
        public long sequence;

        /// int
        public int id;

        /// int64_t->int
        public long start;

        /// int64_t->int
        public long stop;

        /// int
        public int new_chap;

        /// uint8_t->unsigned char
        public byte frametype;

        /// uint16_t->unsigned int
        public uint flags;

        /// int64_t->int
        public long renderOffset;

        /// int
        public int x;

        /// int
        public int y;

        /// int
        public int width;

        /// int
        public int height;

        /// hb_buffer_t*
        public IntPtr sub;

        /// hb_buffer_t*
        public IntPtr next;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_mux_data_s
    {
        /// MP4TrackId->uint32_t->unsigned int
        public uint track;

        /// uint8_t->unsigned char
        public byte subtitle;

        /// int
        public int sub_format;

        /// uint64_t->unsigned int
        public ulong sum_dur;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_interjob_s
    {
        /// int
        public int last_job;

        /// int
        public int frame_count;

        /// uint64_t->unsigned int
        public ulong total_time;

        /// int
        public int render_dropped;

        /// int
        public int vrate;

        /// int
        public int vrate_base;

        /// hb_subtitle_t*
        public IntPtr select_subtitle;
    }

    /// Return Type: void
    ///param0: void*
    internal delegate void hb_thread_s_function(IntPtr param0);

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_thread_s
    {
        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string name;

        /// int
        public int priority;

        /// hb_thread_s_function
        public hb_thread_s_function AnonymousMember1;

        /// void*
        public IntPtr arg;

        /// hb_lock_t*
        public IntPtr @lock;

        /// int
        public int exited;

        /// pthread_t->ptw32_handle_t->Anonymous_55c509b5_bbf2_4788_a684_ac1bd0056655
        public ptw32_handle_t thread;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ptw32_handle_t
    {
        /// void*
        public IntPtr p;

        /// unsigned int
        public uint x;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void LoggingCallback(string message);

    internal partial class HbLib
    {
        [DllImport("hb.dll", EntryPoint = "hb_register_logger", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_register_logger(LoggingCallback callback);

        [DllImport("hb.dll", EntryPoint = "hb_register_error_handler", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_register_error_handler(LoggingCallback callback);

        /// Return Type: hb_handle_t*
        ///verbose: int
        ///update_check: int
        [DllImport("hb.dll", EntryPoint = "hb_init", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_init(int verbose, int update_check);


        /// Return Type: hb_handle_t*
        ///verbose: int
        ///update_check: int
        [DllImport("hb.dll", EntryPoint = "hb_init_dl", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_init_dl(int verbose, int update_check);


        /// Return Type: char*
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_version", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_version(ref hb_handle_s param0);


        /// Return Type: int
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_build", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_get_build(ref hb_handle_s param0);


        /// Return Type: int
        ///h: hb_handle_t*
        ///version: char**
        [DllImport("hb.dll", EntryPoint = "hb_check_update", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_check_update(ref hb_handle_s h, ref IntPtr version);


        /// Return Type: void
        ///param0: hb_handle_t*
        ///param1: int
        [DllImport("hb.dll", EntryPoint = "hb_set_cpu_count", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_cpu_count(ref hb_handle_s param0, int param1);


        /// Return Type: char*
        ///path: char*
        [DllImport("hb.dll", EntryPoint = "hb_dvd_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_dvd_name(IntPtr path);


        /// Return Type: void
        ///enable: int
        [DllImport("hb.dll", EntryPoint = "hb_dvd_set_dvdnav", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_dvd_set_dvdnav(int enable);


        /// Return Type: void
        ///param0: hb_handle_t*
        ///path: char*
        ///title_index: int
        ///preview_count: int
        ///store_previews: int
        [DllImport("hb.dll", EntryPoint = "hb_scan", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_scan(IntPtr hbHandle, [In] [MarshalAs(UnmanagedType.LPStr)] string path, int title_index, int preview_count, int store_previews);


        /// Return Type: hb_list_t*
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_titles", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_get_titles(IntPtr hbHandle);


        /// Return Type: int
        ///buf: hb_buffer_t*
        ///width: int
        ///height: int
        ///color_equal: int
        ///color_diff: int
        ///threshold: int
        ///prog_equal: int
        ///prog_diff: int
        ///prog_threshold: int
        [DllImport("hb.dll", EntryPoint = "hb_detect_comb", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_detect_comb(ref hb_buffer_s buf, int width, int height, int color_equal, int color_diff, int threshold, int prog_equal, int prog_diff, int prog_threshold);

        [DllImport("hb.dll", EntryPoint = "hb_get_preview_by_index", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_get_preview_by_index(IntPtr hbHandle, int title_index, int picture, IntPtr buffer);

        /// Return Type: void
        ///param0: hb_handle_t*
        ///param1: hb_title_t*
        ///param2: int
        ///param3: uint8_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_preview", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_get_preview(IntPtr hbHandle, ref hb_title_s title, int preview, IntPtr buffer);


        /// Return Type: void
        ///param0: hb_job_t*
        ///ratio: double
        ///pixels: int
        [DllImport("hb.dll", EntryPoint = "hb_set_size", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_size(ref hb_job_s param0, double ratio, int pixels);

        [DllImport("hb.dll", EntryPoint = "hb_set_anamorphic_size_by_index", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_anamorphic_size_by_index(IntPtr hbHandle, int title_index, ref int output_width, ref int output_height, ref int output_par_width, ref int output_par_height);

        /// Return Type: void
        ///param0: hb_job_t*
        ///output_width: int*
        ///output_height: int*
        ///output_par_width: int*
        ///output_par_height: int*
        [DllImport("hb.dll", EntryPoint = "hb_set_anamorphic_size", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_anamorphic_size(ref hb_job_s job, ref int output_width, ref int output_height, ref int output_par_width, ref int output_par_height);


        /// Return Type: int
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_count", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_count(IntPtr hbHandle);


        /// Return Type: hb_job_t*
        ///param0: hb_handle_t*
        ///param1: int
        [DllImport("hb.dll", EntryPoint = "hb_job", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hb_job(IntPtr hbHandle, int jobIndex);

        [DllImport("hb.dll", EntryPoint = "hb_set_chapter_name", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_chapter_name(IntPtr hbHandle, int title_index, int chapter_index, [In] [MarshalAs(UnmanagedType.LPStr)] string chapter_name);

        [DllImport("hb.dll", EntryPoint = "hb_set_job", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_set_job(IntPtr hbHandle, int title_index, ref hb_job_s job);

        /// Return Type: void
        ///param0: hb_handle_t*
        ///param1: hb_job_t*
        [DllImport("hb.dll", EntryPoint = "hb_add", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_add(IntPtr hbHandle, ref hb_job_s job);


        /// Return Type: void
        ///param0: hb_handle_t*
        ///param1: hb_job_t*
        [DllImport("hb.dll", EntryPoint = "hb_rem", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_rem(IntPtr hbHandle, IntPtr job);


        /// Return Type: void
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_start", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_start(IntPtr hbHandle);


        /// Return Type: void
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_pause", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_pause(IntPtr hbHandle);


        /// Return Type: void
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_resume", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_resume(IntPtr hbHandle);


        /// Return Type: void
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_stop", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_stop(IntPtr hbHandle);

        [DllImport("hb.dll", EntryPoint = "hb_get_filter_object", CallingConvention = CallingConvention.Cdecl)]
        //public static extern IntPtr hb_get_filter_object(int filter_id, [In] [MarshalAs(UnmanagedType.LPStr)] string settings);
        public static extern IntPtr hb_get_filter_object(int filter_id, IntPtr settings);

        /// Return Type: void
        ///param0: hb_handle_t*
        ///param1: hb_state_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_state", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_get_state(IntPtr hbHandle, ref hb_state_s state);


        /// Return Type: void
        ///param0: hb_handle_t*
        ///param1: hb_state_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_state2", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_get_state2(ref hb_handle_s param0, ref hb_state_s param1);


        /// Return Type: int
        ///param0: hb_handle_t*
        [DllImport("hb.dll", EntryPoint = "hb_get_scancount", CallingConvention = CallingConvention.Cdecl)]
        public static extern int hb_get_scancount(ref hb_handle_s param0);


        /// Return Type: void
        ///param0: hb_handle_t**
        [DllImport("hb.dll", EntryPoint = "hb_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_close(IntPtr hbHandle);

        [DllImport("hb.dll", EntryPoint = "hb_global_close", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hb_global_close();
    }
}
