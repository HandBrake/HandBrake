// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GrayscaleFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the GrayscaleFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.ComponentModel;

    using Caliburn.Micro;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;

    using Action = System.Action;

    public class GrayscaleFilter : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

        public GrayscaleFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;

        }

        public EncodeTask CurrentTask { get; private set; }

        public bool Grayscale
        {
            get
            {
                return this.CurrentTask.Grayscale;
            }

            set
            {
                this.CurrentTask.Grayscale = value;
                this.NotifyOfPropertyChange(() => this.Grayscale);
                this.triggerTabChanged();
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.Grayscale = false;
                return;
            }

            this.Grayscale = preset.Task.Grayscale;

        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.Grayscale);
        }

        public bool MatchesPreset(Preset preset)
        { 
            if (preset.Task.Grayscale != this.Grayscale)
            {
                return false;
            }

            return true;
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }
    }
}
