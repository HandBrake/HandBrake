// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Denoise.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Denoise type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// The denoise.
    /// </summary>
    public enum Denoise
	{
		Off = 0,
        hqdn3d = 1,
		NlMeans = 2,
	}
}
