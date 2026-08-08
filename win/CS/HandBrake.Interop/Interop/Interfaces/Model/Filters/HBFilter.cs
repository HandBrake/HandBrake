// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBFilter.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object representing the key and name of an internal HandBrake filter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Filters
{
    public class HBFilter
    {
        public HBFilter(int filterId, string shortName, string categoryId = null)
        {
            this.FilterId = filterId;
            this.ShortName = shortName;
            this.CategoryId = categoryId;
        }
        
        public int FilterId { get; set; }
        public string ShortName { get; set; }

        public string CategoryId { get; }
    }
}
