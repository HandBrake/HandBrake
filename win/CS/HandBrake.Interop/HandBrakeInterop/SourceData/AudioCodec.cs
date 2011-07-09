// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioCodec.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioCodec type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
	// Only contains 2 real codecs at the moment as those are what we care about. More will be added later.
	public enum AudioCodec
	{
		Ac3,

		Dts,

		Other
	}
}
