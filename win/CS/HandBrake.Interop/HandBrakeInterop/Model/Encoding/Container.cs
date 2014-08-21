// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Container.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Container type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	using System;
	using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The container.
    /// </summary>
    [Flags]
	public enum Container
	{
		None = 0x0,

		[Display(Name = "MP4")]
		MP4,
		[Display(Name = "MKV")]
		MKV
	}
}
