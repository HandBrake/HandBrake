// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StaticPreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Static Preview View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Runtime.ExceptionServices;
    using System.Windows;
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Services.Scan.Interfaces;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    ///     The Static Preview View Model
    /// </summary>
    public class StaticPreviewViewModel : ViewModelBase, IStaticPreviewViewModel
    {
        /*
         * TODO
         * - Integrate Video Preview panel.
         */

        #region Fields

        /// <summary>
        ///     The scan service.
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        ///     The height.
        /// </summary>
        private int height;

        /// <summary>
        ///     The preview image.
        /// </summary>
        private BitmapImage previewImage;

        /// <summary>
        ///     The selected preview image.
        /// </summary>
        private int selectedPreviewImage;

        /// <summary>
        ///     The width.
        /// </summary>
        private int width;

        /// <summary>
        /// The preview not available.
        /// </summary>
        private bool previewNotAvailable;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="StaticPreviewViewModel"/> class.
        /// </summary>
        /// <param name="scanService">
        /// The scan service.
        /// </param>
        public StaticPreviewViewModel(IScan scanService)
        {
            this.scanService = scanService;
            this.selectedPreviewImage = 1;
            this.Title = Properties.Resources.Preview;
            this.PreviewNotAvailable = true;
        }

        #endregion

        #region Public Properties

        /// <summary>
        ///     Gets or sets the height.
        /// </summary>
        public int Height
        {
            get
            {
                return this.height;
            }
            set
            {
                if (value == this.height)
                {
                    return;
                }
                this.height = value;
                this.NotifyOfPropertyChange(() => this.Height);
            }
        }

        /// <summary>
        ///     Gets or sets the preview image.
        /// </summary>
        public BitmapImage PreviewImage
        {
            get
            {
                return this.previewImage;
            }
            set
            {
                if (Equals(value, this.previewImage))
                {
                    return;
                }

                this.previewImage = value;
                this.NotifyOfPropertyChange(() => this.PreviewImage);
            }
        }

        /// <summary>
        ///     Gets or sets the selected preview image.
        /// </summary>
        public int SelectedPreviewImage
        {
            get
            {
                return this.selectedPreviewImage;
            }
            set
            {
                if (value == this.selectedPreviewImage)
                {
                    return;
                }
                this.selectedPreviewImage = value;
                this.NotifyOfPropertyChange(() => this.SelectedPreviewImage);

                this.UpdatePreviewFrame();
            }
        }

        /// <summary>
        ///     Gets or sets the task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        ///     Gets the total previews.
        /// </summary>
        public int TotalPreviews
        {
            get
            {
                return this.UserSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount) - 1;
            }
        }

        /// <summary>
        ///     Gets or sets the width.
        /// </summary>
        public int Width
        {
            get
            {
                return this.width;
            }
            set
            {
                if (value == this.width)
                {
                    return;
                }
                this.width = value;
                this.NotifyOfPropertyChange(() => this.Width);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether preview not available.
        /// </summary>
        public bool PreviewNotAvailable
        {
            get
            {
                return this.previewNotAvailable;
            }
            set
            {
                if (value.Equals(this.previewNotAvailable))
                {
                    return;
                }
                this.previewNotAvailable = value;
                this.NotifyOfPropertyChange(() => this.PreviewNotAvailable);
            }
        }

        #endregion

        #region Public Methods and Operators

        /// <summary>
        /// The update preview frame.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdatePreviewFrame(EncodeTask task)
        {
            this.Task = task;
            this.UpdatePreviewFrame();
            this.DisplayName = "Picture Preview";
            this.Title = Properties.Resources.Preview;
        }

        /// <summary>
        /// Gets or sets a value indicating whether is open.
        /// </summary>
        public bool IsOpen { get; set; }

        /// <summary>
        ///     The update preview frame.
        /// </summary>
        [HandleProcessCorruptedStateExceptions] 
        public void UpdatePreviewFrame()
        {
            // Don't preview for small images.
            if (this.Task.Anamorphic == Anamorphic.Loose && this.Task.Width < 32)
            {
                PreviewNotAvailable = true;
                return;
            }

            if ((this.Task.Anamorphic == Anamorphic.None || this.Task.Anamorphic == Anamorphic.Custom) && (this.Task.Width < 32 || this.Task.Height < 32))
            {
                PreviewNotAvailable = true;
                return;
            }

            BitmapImage image = null;
            try
            {
                image = this.scanService.GetPreview(this.Task, this.SelectedPreviewImage);
            }
            catch (Exception exc)
            {
                PreviewNotAvailable = true;
            }

            if (image != null)
            {
                PreviewNotAvailable = false;
                this.Width = (int)Math.Ceiling(image.Width);
                this.Height = (int)Math.Ceiling(image.Height);
                this.PreviewImage = image;
            }
        }

        /// <summary>
        /// The preview size changed.
        /// </summary>
        /// <param name="ea">
        /// The ea.
        /// </param>
        public void PreviewSizeChanged(SizeChangedEventArgs ea)
        {
            Rect workArea = SystemParameters.WorkArea;
            if (ea.NewSize.Width > workArea.Width)
            {
                this.Width = (int)Math.Round(workArea.Width, 0) - 20;
                this.Title = Properties.Resources.Preview_Scaled;
            }

            if (ea.NewSize.Height > workArea.Height)
            {
                this.Height = (int)Math.Round(workArea.Height, 0) - 20;
                this.Title = Properties.Resources.Preview_Scaled;
            }
        }
        #endregion
    }
}