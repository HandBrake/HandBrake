// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CoverArtPreviewView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.IO;
    using System.Windows;
    using System.Windows.Input;
    using System.Windows.Media.Imaging;

    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Shared;
    using HandBrakeWPF.Utilities.FileDialogs;
    using Strings = HandBrakeWPF.Properties.Resources;

    public partial class CoverArtPreviewView : Window
    {
        private readonly BitmapImage image;
        private readonly Func<RawCoverArtData> loadRawData;

        public CoverArtPreviewView(BitmapImage image, Func<RawCoverArtData> loadRawData)
        {
            this.image = image ?? throw new ArgumentNullException(nameof(image));
            this.loadRawData = loadRawData ?? throw new ArgumentNullException(nameof(loadRawData));
            this.InitializeComponent();
            this.coverArtImage.Source = this.image;
            this.MaxWidth = SystemParameters.WorkArea.Width;
            this.MaxHeight = SystemParameters.WorkArea.Height;
        }

        private void CoverArtImage_OnSizeChanged(object sender, SizeChangedEventArgs e)
        {
            double scale = Math.Min(1, Math.Min(this.coverArtImage.ActualWidth / this.image.Width, this.coverArtImage.ActualHeight / this.image.Height));
            this.Title = scale < 1
                ? string.Format(Strings.CoverArtPreviewView_ScaledTitle, Strings.MainView_Preview, Math.Round(scale * 100))
                : Strings.MainView_Preview;
        }

        private void CoverArtImage_OnMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            this.Close();
        }

        private void SaveImage_OnClick(object sender, RoutedEventArgs e)
        {
            RawCoverArtData rawData = this.loadRawData();
            if (rawData == null || rawData.RawBitmapData == null)
            {
                MessageBox.Show(this, Strings.CoverArtPreviewView_SaveUnavailable, Strings.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            string extension = rawData.CoverArtFileType switch
            {
                CoverArtType.HB_ART_BMP => "bmp",
                CoverArtType.HB_ART_GIF => "gif",
                CoverArtType.HB_ART_PNG => "png",
                CoverArtType.HB_ART_JPEG => "jpg",
                _ => null
            };

            SaveFileDialog dialog = new SaveFileDialog
            {
                Title = Strings.CoverArtPreviewView_SaveAs,
                Filter = extension == null ? "*.*|*.*" : $"*.{extension}|*.{extension}",
                DefaultExt = extension,
                AddExtension = extension != null,
                FileName = "cover-art",
                CheckPathExists = true,
                OverwritePrompt = true
            };

            if (dialog.ShowDialog() != true)
            {
                return;
            }

            try
            {
                File.WriteAllBytes(dialog.FileName, rawData.RawBitmapData);
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                MessageBox.Show(this, string.Format(Strings.CoverArtPreviewView_SaveError, ex.Message), Strings.Error, MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void CoverArtPreviewView_OnPreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                e.Handled = true;
                this.Close();
            }
        }
    }
}