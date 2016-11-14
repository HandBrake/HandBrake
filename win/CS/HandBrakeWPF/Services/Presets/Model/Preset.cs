﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Preset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Preset for encoding with.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Model
{
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Utilities;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using PresetPictureSettingsMode = HandBrakeWPF.Model.Picture.PresetPictureSettingsMode;

    /// <summary>
    /// A Preset for encoding with.
    /// </summary>
    /// <remarks>
    /// Using App Services PropertyChangedBase because Caliburn Micro has [DataContract] on their base class which causes json.net not to serialise properties without [DataContract]
    /// https://github.com/Caliburn-Micro/Caliburn.Micro/issues/89
    /// https://github.com/Caliburn-Micro/Caliburn.Micro/issues/96
    /// </remarks>
    public class Preset : PropertyChangedBase  // Delibery not 
    {
        #region Constants and Fields

        /// <summary>
        /// The is default.
        /// </summary>
        private bool isDefault;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="Preset"/> class. 
        /// </summary>
        public Preset()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Preset"/> class. 
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public Preset(Preset preset)
        {
            this.Category = preset.Category;
            this.Description = preset.Description;
            this.IsBuildIn = preset.IsBuildIn;
            this.Name = preset.Name;
            this.PictureSettingsMode = preset.PictureSettingsMode;
            this.Task = new EncodeTask(preset.Task);
            this.AudioTrackBehaviours = new AudioBehaviours(preset.AudioTrackBehaviours);
            this.SubtitleTrackBehaviours = new SubtitleBehaviours(preset.SubtitleTrackBehaviours);
        }

        #region Properties

        /// <summary>
        /// Gets or sets the category which the preset resides under
        /// </summary>
        public string Category { get; set; }

        /// <summary>
        /// Gets or sets the Description for the preset
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this is a built in preset
        /// </summary>
        public bool IsBuildIn { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether IsDefault.
        /// </summary>
        public bool IsDefault
        {
            get
            {
                return this.isDefault;
            }
            set
            {
                this.isDefault = value;
                this.NotifyOfPropertyChange(() => this.IsDefault);
            }
        }

        /// <summary>
        /// Gets or sets the preset name
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets PictureSettingsMode.
        /// Source Maximum, Custom or None
        /// </summary>
        public PresetPictureSettingsMode PictureSettingsMode { get; set; }

        /// <summary>
        /// Gets or sets task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets the audio track behaviours.
        /// </summary>
        public AudioBehaviours AudioTrackBehaviours { get; set; }

        /// <summary>
        /// Gets or sets the subtitle track behaviours.
        /// </summary>
        public SubtitleBehaviours SubtitleTrackBehaviours { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Update this preset.
        /// The given parameters should be copy-constructed.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="audioBehaviours">
        /// The audio behaviours.
        /// </param>
        /// <param name="subtitleBehaviours">
        /// The subtitle behaviours.
        /// </param>
        public void Update(EncodeTask task, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours)
        {
            // Copy over Max Width / Height for the following picture settings modes.
            if (this.PictureSettingsMode == PresetPictureSettingsMode.Custom
                || this.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
            {
                task.MaxWidth = this.Task.MaxWidth;
                task.MaxHeight = this.Task.MaxHeight;
            }

            this.Task = task;
            this.AudioTrackBehaviours = new AudioBehaviours(audioBehaviours);
            this.SubtitleTrackBehaviours = new SubtitleBehaviours(subtitleBehaviours);
        }

        /// <summary>
        ///  Override the ToString Method
        /// </summary>
        /// <returns>
        /// The Preset Name
        /// </returns>
        public override string ToString()
        {
            return this.Name;
        }

        #endregion

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="other">
        /// The other.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        protected bool Equals(Preset other)
        {
            return string.Equals(this.Name, other.Name);
        }

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="obj">
        /// The obj.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }
            if (ReferenceEquals(this, obj))
            {
                return true;
            }
            if (obj.GetType() != this.GetType())
            {
                return false;
            }
            return Equals((Preset)obj);
        }

        /// <summary>
        /// The get hash code.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        public override int GetHashCode()
        {
            return (this.Name != null ? this.Name.GetHashCode() : 0);
        }
    }
}