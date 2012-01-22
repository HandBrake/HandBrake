// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AdvancedViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Advanced View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding.x264;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Advanced View Model
    /// </summary>
    [Export(typeof(IAdvancedViewModel))]
    public class AdvancedViewModel : ViewModelBase, IAdvancedViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The query.
        /// </summary>
        private string query;

        /// <summary>
        /// The x264 preset.
        /// </summary>
        private x264Preset x264Preset;

        /// <summary>
        /// The x264 profile.
        /// </summary>
        private x264Profile x264Profile;

        /// <summary>
        /// The x264 tune.
        /// </summary>
        private x264Tune x264Tune;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="AdvancedViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public AdvancedViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
        }

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public string Query
        {
            get
            {
                return this.query;
            }
            set
            {
                this.query = value;
                this.NotifyOfPropertyChange(() => this.Query);
            }
        }

        /// <summary>
        /// Gets or sets X264Preset.
        /// </summary>
        public x264Preset X264Preset
        {
            get
            {
                return this.x264Preset;
            }
            set
            {
                this.x264Preset = value;
                this.NotifyOfPropertyChange(() => this.X264Preset);
            }
        }

        /// <summary>
        /// Gets or sets X264Profile.
        /// </summary>
        public x264Profile X264Profile
        {
            get
            {
                return this.x264Profile;
            }
            set
            {
                this.x264Profile = value;
                this.NotifyOfPropertyChange(() => this.X264Profile);
            }
        }

        /// <summary>
        /// Gets or sets X264Tune.
        /// </summary>
        public x264Tune X264Tune
        {
            get
            {
                return this.x264Tune;
            }
            set
            {
                this.x264Tune = value;
                this.NotifyOfPropertyChange(() => this.X264Tune);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Title title, Preset preset, EncodeTask task)
        {
            this.Query = preset.Task.AdvancedEncoderOptions;
            this.X264Preset = preset.Task.x264Preset;
            this.X264Profile = preset.Task.x264Profile;
            this.X264Tune = preset.Task.X264Tune;
        }

        #endregion
    }
}