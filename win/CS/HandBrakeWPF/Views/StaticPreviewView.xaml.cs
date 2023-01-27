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
    using System;
    using System.Windows;
    using System.Windows.Input;
    using System.Windows.Media.Imaging;

    using HandBrakeWPF.ViewModels;
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
            this.InitializeComponent();

            this.SizeChanged += this.StaticPreviewView_SizeChanged;
            this.Title = Properties.Resources.Preview;

            this.videoPlayer.MediaFailed += this.VideoPlayer_MediaFailed;
        }

        private void VideoPlayer_MediaFailed(object sender, ExceptionRoutedEventArgs e)
        {
            ((StaticPreviewViewModel)this.DataContext).HandleMediaError(e.ErrorException);
        }
         
        private void StaticPreviewView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            // Prevent the Window Growing Past Screen Bounds
            Rect workArea = SystemParameters.WorkArea;
            if (e.NewSize.Width > workArea.Width)
            {
                this.Width = (int)Math.Round(workArea.Width, 0) - 50;
            }

            if (e.NewSize.Height > workArea.Height)
            {
                this.Height = (int)Math.Round(workArea.Height, 0) - 50;
            }

            // Update Window title scale factor.
            this.UpdateWindowTitle();
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

        private void UpdateWindowTitle()
        {
            if (((StaticPreviewViewModel)this.DataContext).IsMediaPlayerVisible)
            {
                this.Title = Properties.Resources.StaticPreviewView_VideoPreview;
                return;
            }

            BitmapSource image = ((IStaticPreviewViewModel)this.DataContext).PreviewImage;
            if (image != null && this.previewImage != null && this.previewImage.ActualWidth > 0)
            {
                double origWidth = Math.Round(image.Width, 0);
                double origHeight = Math.Round(image.Height, 0);

                double actualWidth = Math.Round(this.previewImage.ActualWidth, 0);
                double actualHeight = Math.Round(this.previewImage.ActualHeight, 0);

                double scaleW = actualWidth / origWidth;
                double scaleH = actualHeight / origHeight;

                double scaleFactor = Math.Min(scaleW, scaleH);

                double scalePercentage = Math.Round(100 * scaleFactor, 0);

                this.Title = string.Format(Properties.Resources.StaticPreviewView_Title, scalePercentage);
            }
            else
            {
                this.Title = Properties.Resources.Preview;
            }
        }

        private void PlayVideo_OnClick(object sender, RoutedEventArgs e)
        {
            this.UpdateWindowTitle();
            this.videoPlayer.Stop();
            this.videoPlayer.Close();
            ((StaticPreviewViewModel)this.DataContext).Play();

            if (!((StaticPreviewViewModel)this.DataContext).UseExternalPlayer)
            {
                this.videoPlayer.Play();
            }
        }

        private void ChangeMediaVolume(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            this.videoPlayer.Volume = (double)volumeSlider.Value;
        }
    }
}
