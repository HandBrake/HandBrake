// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CropPanelView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Standalone
{
    using System.Windows.Controls;
    using System.Windows.Input;

    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    public partial class CropPanelView : UserControl
    { 
        public CropPanelView()
        {
            this.InitializeComponent();
        }

        private void UIElement_OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (this.DataContext != null && this.DataContext is PictureSettingsViewModel)
            {
               var viewModel = ((PictureSettingsViewModel)this.DataContext);
               IStaticPreviewViewModel previewViewModel = viewModel.StaticPreviewViewModel;
               if (previewViewModel != null)
               {
                   previewViewModel.ShowPictureSettingControls = false;
               }
            }
        }
    }
}
