using System.Runtime.InteropServices;

namespace HandBrake.Interop
{
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
	public struct hb_chapter_s
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
		public ulong block_start;

		/// int
		public ulong block_end;

		/// int
		public ulong block_count;

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
}
