// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsUpdates.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Options
{
    using System.Windows.Controls;
    using System.Windows.Input;

    public partial class OptionsUpdates : UserControl
    {
        public OptionsUpdates()
        {
            InitializeComponent();
        }

        private void VersionTextBox_OnMouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.versionTextBox.SelectAll();
        }
    }
}
