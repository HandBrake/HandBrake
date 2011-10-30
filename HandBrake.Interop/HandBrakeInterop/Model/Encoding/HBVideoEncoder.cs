// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBVideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	public class HBVideoEncoder
	{
		public string ShortName { get; set; }

		public string DisplayName { get; set; }

		public int Id { get; set; }

		public Container CompatibleContainers { get; set; }
	}
}
