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
		public const int JobPaddingBytes = 24688;
		public const int AudioPaddingBytes = 24632;
#else
		public const int JobPaddingBytes = 24640;
		public const int AudioPaddingBytes = 24604;
#endif
	}
}
