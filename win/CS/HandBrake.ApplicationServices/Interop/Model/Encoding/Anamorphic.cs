// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Anamorphic.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Anamorphic type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
	using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The anamorphic.
    /// </summary>
    public enum Anamorphic
	{
		[Display(Name = "None")]
		None = 0,
		[Display(Name = "Strict")]
		Strict = 1,
		[Display(Name = "Loose")]
		Loose = 2,
		[Display(Name = "Custom")]
		Custom = 3
	}
}