// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MarshalingConstants.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the MarshalingConstants type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

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
