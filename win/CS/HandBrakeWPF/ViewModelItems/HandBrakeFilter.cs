// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Model that represents a HandBrake Filter at the UI level.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems
{
    using HandBrake.App.Core.Extensions;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;

    using HandBrakeWPF.Properties;

    public class HandBrakeFilter
    {
        
        public HandBrakeFilter(HBFilter filter)
        {
            FilterId = filter.FilterId;
            ShortName = filter.ShortName;

            string resourceName = "Filter_" + filter.ShortName;
            this.DisplayName = Resources.ResourceManager.GetString(resourceName)
                               ?? filter.ShortName?.ToTitleCase();

            string categoryName = "Filter_Category_" + filter.CategoryId;
            this.Category = Resources.ResourceManager.GetString(categoryName)
                            ?? filter.CategoryId?.ToTitleCase();

        }
        
        public int FilterId { get; private set; }
        
        public string ShortName { get; private set; }

        public string DisplayName { get; private set; }
        
        public string Category { get; private set; }
    }
}
