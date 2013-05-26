// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceMenuItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SourceMenuItem type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System.Collections.ObjectModel;
    using System.Windows.Controls;
    using System.Windows.Input;

    /// <summary>
    /// The source menu item.
    /// </summary>
    public class SourceMenuItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceMenuItem"/> class.
        /// </summary>
        public SourceMenuItem()
        {
            this.Children = new ObservableCollection<SourceMenuItem>();
        }

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text { get; set; }

        /// <summary>
        /// Gets or sets the command.
        /// </summary>
        public ICommand Command { get; set; }

        /// <summary>
        /// Gets or sets the image.
        /// </summary>
        public Image Image { get; set; }

        /// <summary>
        /// Gets or sets the children.
        /// </summary>
        public ObservableCollection<SourceMenuItem> Children { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is drive.
        /// </summary>
        public bool IsDrive { get; set; }

        /// <summary>
        /// Gets or sets the tag.
        /// </summary>
        public object Tag { get; set; }
    }
}
