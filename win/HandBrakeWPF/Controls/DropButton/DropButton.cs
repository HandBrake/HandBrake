// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DropButton.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DropDownButton type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls.DropButton
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Controls.Primitives;
    using System.Windows.Data;

    /// <summary>
    /// The drop down button.
    /// </summary>
    public class DropButton : ToggleButton
    {
        /// <summary>
        /// The drop down property.
        /// </summary>
        public static readonly DependencyProperty DropDownProperty =
          DependencyProperty.Register("DropDown",
                                      typeof(ContextMenu),
                                      typeof(DropButton),
                                      new UIPropertyMetadata(null));

        /// <summary>
        /// Initializes a new instance of the <see cref="DropButton"/> class.
        /// </summary>
        public DropButton()
        {
            // Bind the ToogleButton.IsChecked property to the drop-down's IsOpen property 
            Binding binding = new Binding("DropDown.IsOpen") { Source = this };
            this.SetBinding(IsCheckedProperty, binding);
        }

        /// <summary>
        /// Gets or sets the drop down.
        /// </summary>
        public ContextMenu DropDown
        {
            get { return (ContextMenu)this.GetValue(DropDownProperty); }
            set { this.SetValue(DropDownProperty, value); }
        }

        /// <summary>
        /// Handle the users click on the button.
        /// </summary>
        protected override void OnClick()
        {
            if (this.DropDown != null)
            {
                // If there is a drop-down assigned to this button, then position and display it 

                this.DropDown.PlacementTarget = this;
                this.DropDown.Placement = PlacementMode.Bottom;
                this.DropDown.IsOpen = true;
            }
        }
    }
}
