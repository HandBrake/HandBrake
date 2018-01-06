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
        /// Gets or sets the children.
        /// </summary>
        public ObservableCollection<SourceMenuItem> Children { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is drive.
        /// </summary>
        public bool IsDrive { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is open folder.
        /// </summary>
        public bool IsOpenFolder { get; set; }

        /// <summary>
        /// Gets a value indicating whether is open file.
        /// </summary>
        public bool IsOpenFile
        {
            get
            {
                return !this.IsOpenFolder && !this.IsDrive && (this.Children == null || this.Children.Count == 0);
            }
        }

        /// <summary>
        /// Gets or sets the tag.
        /// </summary>
        public object Tag { get; set; }

        /// <summary>
        /// Gets or sets the input gesture text.
        /// </summary>
        public string InputGestureText { get; set; }
    }
}
