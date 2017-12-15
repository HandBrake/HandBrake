// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StaticPreviewView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for StaticPreviewView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Input;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Interaction logic for StaticPreviewView.xaml
    /// </summary>
    public partial class StaticPreviewView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="StaticPreviewView"/> class.
        /// </summary>
        public StaticPreviewView()
        {
            InitializeComponent();
        }

        private void PreviewImage_OnMouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (e.Delta > 1)
            {
                ((IStaticPreviewViewModel)this.DataContext).NextPreview();
            }
            else
            {
                ((IStaticPreviewViewModel)this.DataContext).PreviousPreview();
            }
        }
    }
}
