// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DetelecineItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DetelecineItem type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.Collections.Generic;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

    using Action = System.Action;

    public class DetelecineItem : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

        public DetelecineItem(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
        }

        public EncodeTask CurrentTask { get; private set; }

        public Detelecine SelectedDetelecine
        {
            get
            {
                return this.CurrentTask.Detelecine;
            }

            set
            {
                this.CurrentTask.Detelecine = value;
                this.NotifyOfPropertyChange(() => this.SelectedDetelecine);

                // Show / Hide the Custom Control
                if (value != Detelecine.Custom) this.CustomDetelecine = string.Empty;
                this.NotifyOfPropertyChange(() => this.ShowDetelecineCustom);
                this.triggerTabChanged();
            }
        }

        public bool ShowDetelecineCustom => this.CurrentTask.Detelecine == Detelecine.Custom;

        public IEnumerable<Detelecine> DetelecineOptions => EnumHelper<Detelecine>.GetEnumList();

        public string CustomDetelecine
        {
            get
            {
                return this.CurrentTask.CustomDetelecine;
            }

            set
            {
                this.CurrentTask.CustomDetelecine = value;
                this.NotifyOfPropertyChange(() => this.CustomDetelecine);
                this.triggerTabChanged();
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedDetelecine = Detelecine.Off;
                return;
            }

            this.SelectedDetelecine = preset.Task.Detelecine;
            this.CustomDetelecine = preset.Task.CustomDetelecine;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.SelectedDetelecine);
            this.NotifyOfPropertyChange(() => this.CustomDetelecine);
            this.NotifyOfPropertyChange(() => this.ShowDetelecineCustom);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.Detelecine != this.SelectedDetelecine)
            {
                return false;
            }

            if (preset.Task.CustomDetelecine != this.CustomDetelecine)
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
