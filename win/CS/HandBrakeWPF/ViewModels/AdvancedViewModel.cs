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
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Advanced View Model
    /// </summary>
    public class AdvancedViewModel : ViewModelBase, IAdvancedViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for displaying x264 options
        /// </summary>
        private bool? displayX264Options;

        /// <summary>
        /// The show x 264 panel.
        /// </summary>
        private bool showX264Panel;

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets the x 264 view model.
        /// </summary>
        public IX264ViewModel X264ViewModel { get; set; }

        /// <summary>
        /// Gets or sets the encoder options view model.
        /// </summary>
        public IEncoderOptionsViewModel EncoderOptionsViewModel { get; set; }

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

        /// <summary>
        /// Gets or sets a value indicating whether DisplayX264Options.
        /// </summary>
        public bool? ShowSimplePanel
        {
            get
            {
                return this.displayX264Options;
            }
            set
            {
                this.displayX264Options = value;
                this.NotifyOfPropertyChange(() => this.ShowSimplePanel);
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
            this.EncoderOptionsViewModel.SetEncoder(encoder);
            this.X264ViewModel.SetEncoder(encoder);

            if (encoder == VideoEncoder.X264)
            {
                this.ShowX264Panel = true;
                this.ShowSimplePanel = false;
            }
            else
            {
                this.ShowX264Panel = false;
                this.ShowSimplePanel = true;
            }  
        }

        /// <summary>
        /// The clear.
        /// </summary>
        public void Clear()
        {
            this.EncoderOptionsViewModel.Clear();
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
            this.EncoderOptionsViewModel.SetPreset(preset, task);
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
            this.EncoderOptionsViewModel.UpdateTask(task);
            this.X264ViewModel.UpdateTask(task);

            this.SetEncoder(task.VideoEncoder);
        }

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
            this.EncoderOptionsViewModel.SetSource(title, preset, task);
            this.X264ViewModel.SetSource(title, preset, task);
        }

        #endregion
    }
}