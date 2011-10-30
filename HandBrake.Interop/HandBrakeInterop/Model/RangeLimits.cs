// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RangeLimits.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
	public class RangeLimits
	{
		public float Low { get; set; }
		public float High { get; set; }
		public float Granularity { get; set; }
		public bool Ascending { get; set; }
	}
}
