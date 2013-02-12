// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleSource.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SubtitleSource type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
    /// <summary>
    /// The subtitle source.
    /// </summary>
    public enum SubtitleSource
	{
		VobSub,
		SRT,
		CC608,
		CC708,
		UTF8,
		TX3G,
		SSA,
		PGS
	}
}
