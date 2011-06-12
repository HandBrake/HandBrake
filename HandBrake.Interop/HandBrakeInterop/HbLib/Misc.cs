using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace HandBrake.Interop
{
	[StructLayout(LayoutKind.Sequential)]
	public struct hb_anamorphic_substruct
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
	public struct hb_list_s
	{
		/// void**
		public IntPtr items;

		/// int
		public int items_alloc;

		/// int
		public int items_count;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_rate_s
	{
		/// char*
		[MarshalAs(UnmanagedType.LPStr)]
		public string @string;

		/// int
		public int rate;
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
	public struct hb_metadata_s
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

	public enum hb_title_type_anon
	{
		HB_DVD_TYPE,

		HB_BD_TYPE,

		HB_STREAM_TYPE,

		HB_FF_STREAM_TYPE,
	}

	public enum Anonymous_618ebeca_0ad9_4a71_9a49_18e50ac2e9db
	{
		/// HB_MPEG2_PS_DEMUXER -> 0
		HB_MPEG2_PS_DEMUXER = 0,

		HB_MPEG2_TS_DEMUXER,

		HB_NULL_DEMUXER,
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_state_scanning_anon
	{
		/// int
		public int title_cur;

		/// int
		public int title_count;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_state_working_anon
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
	public struct hb_state_workdone_anon
	{
		/// int
		public int error;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_state_muxing_anon
	{
		/// float
		public float progress;
	}

	[StructLayout(LayoutKind.Explicit)]
	public struct hb_state_param_u
	{
		[FieldOffset(0)]
		public hb_state_scanning_anon scanning;

		[FieldOffset(0)]
		public hb_state_working_anon working;

		[FieldOffset(0)]
		public hb_state_workdone_anon workdone;

		[FieldOffset(0)]
		public hb_state_muxing_anon muxing;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_state_s
	{
		/// int
		public int state;
		public hb_state_param_u param;
	}

	[StructLayout(LayoutKind.Explicit)]
	public struct Anonymous_a0a59d69_d9a4_4003_a198_f7c51511e31d
	{
		/// int
		[FieldOffset(0)]
		public int ac3;

		/// int
		[FieldOffset(0)]
		public int dca;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_mixdown_s
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

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_fifo_s
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
	public struct hb_lock_s
	{
	}

	// Only called by detect_comb at the moment
	[StructLayout(LayoutKind.Sequential)]
	public struct hb_buffer_s
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

		public hb_buffer_type_anon type;

		/// int
		public int id;

		/// int64_t->int
		public long start;

		/// int64_t->int
		public long stop;

		public long pcr;

		public byte discontinuity;

		/// int
		public int new_chap;

		/// uint8_t->unsigned char
		public byte frametype;

		// Given uint by default, probably should be ushort?
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

	public enum hb_buffer_type_anon
	{
		AUDIO_BUF,

		VIDEO_BUF,

		SUBTITLE_BUF,

		OTHER_BUF
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_mux_data_s
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

	// Not referred to anywhere
	[StructLayout(LayoutKind.Sequential)]
	public struct hb_interjob_s
	{
		/// int
		public int last_job;

		/// int
		public int frame_count;

		public int out_frame_count;

		/// uint64_t->unsigned int
		public ulong total_time;

		/// int
		public int vrate;

		/// int
		public int vrate_base;

		/// hb_subtitle_t*
		public IntPtr select_subtitle;
	}

	/// Return Type: void
	///param0: void*
	public delegate void hb_thread_s_function(IntPtr param0);

	[StructLayout(LayoutKind.Sequential)]
	public struct hb_thread_s
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
	public struct ptw32_handle_t
	{
		/// void*
		public IntPtr p;

		/// unsigned int
		public uint x;
	}

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void LoggingCallback(string message);
}
