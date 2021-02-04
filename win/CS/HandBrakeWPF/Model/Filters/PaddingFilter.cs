// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PaddingFilter.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PaddingFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    public class PaddingFilter
    {
        public bool Enabled { get; set; }

        public string Color { get; set; }

        public int X { get; set; }

        public int Y { get; set; }
    }
}
