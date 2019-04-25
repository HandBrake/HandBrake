// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DeblockFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DeblockFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.Globalization;

    using Caliburn.Micro;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;

    using Action = System.Action;

    public class DeblockFilter : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

        public DeblockFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
            this.DeblockValue = 4; // OFF
        }

        public EncodeTask CurrentTask { get; private set; }

        public string DeblockText =>
            this.DeblockValue == 4 ? "Off" : this.DeblockValue.ToString(CultureInfo.InvariantCulture);

        public int DeblockValue
        {
            get
            {
                return this.CurrentTask.Deblock;
            }

            set
            {
                this.CurrentTask.Deblock = value;
                this.NotifyOfPropertyChange(() => this.DeblockValue);
                this.NotifyOfPropertyChange(() => this.DeblockText);
                this.triggerTabChanged();
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.DeblockValue = 4; // OFF
                return;
            }

            this.DeblockValue = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.DeblockValue);
        }

        public bool MatchesPreset(Preset preset)
        {
            int presetDeblock = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;

            if (presetDeblock != this.DeblockValue)
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
