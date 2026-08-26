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
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media.Imaging;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    public partial class StaticPreviewView : Window
    {
        private bool isDraggingCropBorder;
        private Point cropBorderDragStartPoint;
        private bool hasUserPositionedCropBorder;
        
        public StaticPreviewView()
        {
            this.InitializeComponent();

            this.SizeChanged += this.StaticPreviewView_SizeChanged;
            this.Title = Properties.Resources.Preview;

            this.videoPlayer.MediaFailed += this.VideoPlayer_MediaFailed;

            this.InputBindings.Add(new InputBinding(new CloseWindowCommand(this), new KeyGesture(Key.W, ModifierKeys.Control))); // Close Window
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }


        private void CropSettingsBorder_OnLoaded(object sender, RoutedEventArgs e)
        {
            this.CenterCropSettingsBorder();
            this.cropSettingsBorder.IsVisibleChanged += this.CropSettingsBorder_OnIsVisibleChanged;
        }

        private void CropSettingsBorder_OnIsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.cropSettingsBorder.IsVisible)
            {
                this.CenterCropSettingsBorder();
            }
        }

        private void CropSettingsBorder_OnSizeChanged(object sender, SizeChangedEventArgs e)
        {
            this.CenterCropSettingsBorder();
        }

        private void DragCanvas_OnSizeChanged(object sender, SizeChangedEventArgs e)
        {
            this.CenterCropSettingsBorder();
        }

        private void CenterCropSettingsBorder()
        {
            if (this.hasUserPositionedCropBorder)
            {
                return;
            }

            if (this.dragCanvas.ActualWidth > 0 && this.cropSettingsBorder.ActualWidth > 0)
            {
                double left = (this.dragCanvas.ActualWidth - this.cropSettingsBorder.ActualWidth) / 2;
                double top = (this.dragCanvas.ActualHeight - this.cropSettingsBorder.ActualHeight) / 2;

                Canvas.SetLeft(this.cropSettingsBorder, Math.Max(0, left));
                Canvas.SetTop(this.cropSettingsBorder, Math.Max(0, top));
            }
        }

        private void CropSettingsBorder_OnMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            this.isDraggingCropBorder = true;
            this.cropBorderDragStartPoint = e.GetPosition(this.dragCanvas);
            this.cropSettingsBorder.CaptureMouse();
        }

        private void CropSettingsBorder_OnMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            this.isDraggingCropBorder = false;
            this.cropSettingsBorder.ReleaseMouseCapture();
        }

        private void CropSettingsBorder_OnMouseMove(object sender, MouseEventArgs e)
        {
            if (!this.isDraggingCropBorder || e.LeftButton != MouseButtonState.Pressed)
            {
                return;
            }

            this.hasUserPositionedCropBorder = true;

            Point currentPosition = e.GetPosition(this.dragCanvas);
            double deltaX = currentPosition.X - this.cropBorderDragStartPoint.X;
            double deltaY = currentPosition.Y - this.cropBorderDragStartPoint.Y;

            double newLeft = Canvas.GetLeft(this.cropSettingsBorder);
            double newTop = Canvas.GetTop(this.cropSettingsBorder);

            if (double.IsNaN(newLeft))
            {
                newLeft = 0;
            }

            if (double.IsNaN(newTop))
            {
                newTop = 0;
            }

            newLeft += deltaX;
            newTop += deltaY;

            double maxLeft = Math.Max(0, this.dragCanvas.ActualWidth - this.cropSettingsBorder.ActualWidth);
            double maxTop = Math.Max(0, this.dragCanvas.ActualHeight - this.cropSettingsBorder.ActualHeight);

            newLeft = Math.Min(Math.Max(0, newLeft), maxLeft);
            newTop = Math.Min(Math.Max(0, newTop), maxTop);

            Canvas.SetLeft(this.cropSettingsBorder, newLeft);
            Canvas.SetTop(this.cropSettingsBorder, newTop);

            this.cropBorderDragStartPoint = currentPosition;
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
