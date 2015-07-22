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
    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Advanced View Model
    /// </summary>
    public class AdvancedViewModel : ViewModelBase, IAdvancedViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The show x 264 panel.
        /// </summary>
        private bool showX264Panel;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="AdvancedViewModel"/> class.
        /// </summary>
        /// <param name="x264ViewModel">
        /// The x 264 view model.
        /// </param>
        public AdvancedViewModel(IX264ViewModel x264ViewModel)
        {
            this.X264ViewModel = x264ViewModel;
        }

        #region Properties

        /// <summary>
        /// Gets or sets the x 264 view model.
        /// </summary>
        public IX264ViewModel X264ViewModel { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether show x 264 panel.
        /// </summary>
        public bool ShowX264Panel
        {
            get
            {
                return this.showX264Panel;
            }
            set
            {
                this.showX264Panel = value;
                this.NotifyOfPropertyChange(() => this.ShowX264Panel);
            }
        }

        #endregion

        #region Implemented Interfaces

        /// <summary>
        /// The set encoder.
        /// </summary>
        /// <param name="encoder">
        /// The encoder.
        /// </param>
        public void SetEncoder(VideoEncoder encoder)
        {
            this.X264ViewModel.SetEncoder(encoder);
            this.ShowX264Panel = encoder == VideoEncoder.X264;
        }

        /// <summary>
        /// The clear.
        /// </summary>
        public void Clear()
        {
            this.X264ViewModel.Clear();
        }

        /// <summary>
        /// Setup this tab for the specified preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.X264ViewModel.SetPreset(preset, task);
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask task)
        {
            this.X264ViewModel.UpdateTask(task);

            this.SetEncoder(task.VideoEncoder);
        }

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.X264ViewModel.SetSource(source, title, preset, task);
        }

        #endregion
    }
}