// --------------------------------------------------------------------------------------------------------------------
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
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.ViewModels;

    using EncodeTask = Encode.Model.EncodeTask;

    public class Preset : PropertyChangedBase, IPresetObject
    {
        private bool isDefault;
        private bool isSelected;

        public Preset()
        {
        }

        public Preset(Preset preset)
        {
            this.Category = preset.Category;
            this.Description = preset.Description;
            this.Name = preset.Name;
            this.IsExpanded = preset.IsExpanded;
            this.IsSelected = preset.IsSelected;
            this.IsBuildIn = preset.IsBuildIn;
            this.IsDefault = preset.IsDefault;
            this.Task = new EncodeTask(preset.Task);
            this.IsPresetDisabled = preset.IsPresetDisabled;
            this.AudioTrackBehaviours = new AudioBehaviours(preset.AudioTrackBehaviours);
            this.SubtitleTrackBehaviours = new SubtitleBehaviours(preset.SubtitleTrackBehaviours);
        }

        public string Category { get; set; }

        public string Description { get; set; }

        public bool IsExpanded { get; set; }

        public string DisplayValue => string.Format("{0}", this.Name);

        public bool IsSelected
        {
            get => this.isSelected;
            set
            {
                if (value == this.isSelected) return;
                this.isSelected = value;
                this.NotifyOfPropertyChange(() => this.IsSelected);
            }
        }

        public bool IsBuildIn { get; set; }

        public bool IsDefault
        {
            get => this.isDefault;

            set
            {
                this.isDefault = value;
                this.NotifyOfPropertyChange(() => this.IsDefault);
            }
        }

        public string Name { get; set; }

        public EncodeTask Task { get; set; }

        public AudioBehaviours AudioTrackBehaviours { get; set; }

        public SubtitleBehaviours SubtitleTrackBehaviours { get; set; }

        public bool IsPresetDisabled { get; set; }

        public void Update(EncodeTask task, AudioBehaviours audioBehaviours, SubtitleBehaviours subtitleBehaviours)
        {
            this.Task = task;
            this.AudioTrackBehaviours = new AudioBehaviours(audioBehaviours);
            this.SubtitleTrackBehaviours = new SubtitleBehaviours(subtitleBehaviours);
        }

        public override string ToString()
        {
            return this.Name;
        }
    }
}