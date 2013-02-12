// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InputType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the InputType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The input type.
    /// </summary>
    public enum InputType
	{
		[Display(Name = "File")]
		Stream,

		[Display(Name = "DVD")]
		Dvd,

		[Display(Name = "Blu-ray")]
		Bluray
	}
}
