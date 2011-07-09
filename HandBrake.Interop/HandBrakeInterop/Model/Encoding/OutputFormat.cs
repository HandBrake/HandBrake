// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OutputFormat.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the OutputFormat type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    public enum OutputFormat
	{
		[Display(Name = "MP4")]
		Mp4,
		[Display(Name = "MKV")]
		Mkv
	}
}
