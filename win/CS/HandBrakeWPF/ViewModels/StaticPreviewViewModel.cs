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
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    ///     The Static Preview View Model
    /// </summary>
    public class StaticPreviewViewModel : ViewModelBase, IStaticPreviewViewModel
    {
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

        #endregion

        #region Public Methods and Operators

        /// <summary>
        /// The preview frame.
        /// </summary>
        /// <param name="image">
        /// The image.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void PreviewFrame(BitmapImage image, EncodeTask task)
        {
            this.Task = task;
            this.Width = (int)Math.Ceiling(image.Width);
            this.Height = (int)Math.Ceiling(image.Height);
            this.PreviewImage = image;
        }

        protected override void OnActivate()
        {
            Console.Write("test");
            base.OnActivate();
        }

        /// <summary>
        ///     The update preview frame.
        /// </summary>
        public void UpdatePreviewFrame()
        {
            BitmapImage image = this.scanService.GetPreview(this.Task, this.SelectedPreviewImage);

            if (image != null)
            {
                this.PreviewFrame(image, this.Task);
            }
        }

        #endregion
    }
}