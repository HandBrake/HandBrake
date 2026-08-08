// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FilterMenuItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the FilterMenuItem type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System.Collections.ObjectModel;

    using HandBrakeWPF.ViewModelItems;

    /// <summary>
    /// The filter menu item.
    /// </summary>
    public class FilterMenuItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FilterMenuItem"/> class.
        /// </summary>
        public FilterMenuItem()
        {
            this.Children = new ObservableCollection<FilterMenuItem>();
        }

        /// <summary>
        /// Gets or sets the header text.
        /// </summary>
        public string Header { get; set; }

        /// <summary>
        /// Gets or sets the filter (null for category nodes).
        /// </summary>
        public HandBrakeFilter Filter { get; set; }

        /// <summary>
        /// Gets or sets the children menu items.
        /// </summary>
        public ObservableCollection<FilterMenuItem> Children { get; set; }
    }
}
