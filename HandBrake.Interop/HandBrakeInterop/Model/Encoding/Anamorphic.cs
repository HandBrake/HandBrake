// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Anamorphic.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Anamorphic type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    public enum Anamorphic
	{
		[Display(Name = "None")]
		None = 0,
		[Display(Name = "Strict")]
		Strict,
		[Display(Name = "Loose")]
		Loose,
		[Display(Name = "Custom")]
		Custom
	}
}
