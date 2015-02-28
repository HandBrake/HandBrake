// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Deinterlace.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Deinterlace type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    /// <summary>
    /// The deinterlace.
    /// </summary>
    public enum Deinterlace
	{
		Off = 0,
		Fast = 2,
		Slow = 3,
		Slower = 4,
		Bob = 5,
		Custom = 1
	}
}
