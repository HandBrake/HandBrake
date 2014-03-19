// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MarshalingConstants.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the MarshalingConstants type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Helpers
{
    /// <summary>
    /// The marshaling constants.
    /// </summary>
    public static class MarshalingConstants
	{
#if X64
		public const int JobPaddingBytes = 49272;
		public const int AudioPaddingBytes = 49208;
#else
		/// <summary>
	    /// Job Padding Bytes
	    /// </summary>
		public const int JobPaddingBytes = 49220;

	    /// <summary>
	    /// Audio Padding Bytes
	    /// </summary>
	    public const int AudioPaddingBytes = 49180;
#endif
	}
}
