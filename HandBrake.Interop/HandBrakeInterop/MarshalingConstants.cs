using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
	public static class MarshalingConstants
	{
#if X64
		public const int JobPaddingBytes = 24696;
		public const int AudioPaddingBytes = 24640;
#else
		public const int JobPaddingBytes = 24644;
		public const int AudioPaddingBytes = 24608;
#endif
	}
}
