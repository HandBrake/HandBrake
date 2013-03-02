// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncoderOptionsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Simple Encoder options screen
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Simple Encoder options screen
    /// </summary>
    public class EncoderOptionsViewModel : ViewModelBase, IEncoderOptionsViewModel, ITabInterface
    {
        /// <summary>
        /// The cached options.
        /// </summary>
        private readonly Dictionary<VideoEncoder, string> cachedOptions = new Dictionary<VideoEncoder, string>();

        /// <summary>
        /// Initializes a new instance of the <see cref="EncoderOptionsViewModel"/> class.
        /// </summary>
        public EncoderOptionsViewModel()
        {
            this.Task = new EncodeTask();
        }

        /// <summary>
        /// Gets or sets the task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets the preset.
        /// </summary>
        public Preset Preset { get; set; }

        /// <summary>
        /// Gets or sets the current video encoder.
        /// </summary>
        public VideoEncoder CurrentVideoEncoder { get; set; }

        /// <summary>
        /// Gets or sets the options string.
        /// </summary>
        public string AdvancedOptionsString
        {
            get
            {
                return this.Task.AdvancedEncoderOptions;
            }
            set
            {
                this.Task.AdvancedEncoderOptions = value;
                this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);
            }
        }

        /// <summary>
        /// The set source.
        /// </summary>
        /// <param name="selectedTitle">
        /// The selected title.
        /// </param>
        /// <param name="currentPreset">
        /// The current preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Title selectedTitle, Preset currentPreset, EncodeTask task)
        {
            this.Task = task;
            this.Preset = currentPreset;
            this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);
        }

        /// <summary>
        /// The set preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.Task = task;
            this.Preset = preset;
            this.AdvancedOptionsString = preset.Task.AdvancedEncoderOptions;
        }

        /// <summary>
        /// The update task.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask task)
        {        
            this.Task = task;
            this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);
        }

        /// <summary>
        /// The set encoder.
        /// </summary>
        /// <param name="encoder">
        /// The encoder.
        /// </param>
        public void SetEncoder(VideoEncoder encoder)
        {
            // Cache the existing string so it can be reused if the user changes the encoder back.
            if (!string.IsNullOrEmpty(this.AdvancedOptionsString))
            {
                this.cachedOptions[CurrentVideoEncoder] = this.AdvancedOptionsString;
            }

            this.CurrentVideoEncoder = encoder;

            // Set the option from the cached version if we have one.
            string advacnedOptionsString;
            if (cachedOptions.TryGetValue(encoder, out advacnedOptionsString)
                && !string.IsNullOrEmpty(advacnedOptionsString))
            {
                this.AdvancedOptionsString = advacnedOptionsString;
            }
            else
            {
                this.AdvancedOptionsString = this.Preset != null && this.Preset.Task != null
                                             && !string.IsNullOrEmpty(this.Preset.Task.AdvancedEncoderOptions)
                                                 ? this.Preset.Task.AdvancedEncoderOptions
                                                 : string.Empty;
            }
        }

        /// <summary>
        /// The clear.
        /// </summary>
        public void Clear()
        {
        }
    }
}
