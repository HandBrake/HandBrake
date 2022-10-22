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
    using System.ComponentModel;
    using System.Linq;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

    using Action = System.Action;

    public class DeblockFilter : PropertyChangedBase
    {
        public static readonly string Off = "off";
        public static readonly string Custom = "custom";

        private readonly Action triggerTabChanged;

        public DeblockFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;

            this.SetPresets();
            this.SetTunes();
        }

        public EncodeTask CurrentTask { get; private set; }

        public object Presets { get; set; }

        public object Tunes { get; set; }

        public bool ShowDeblockTune => this.SelectedPreset != null && this.SelectedPreset.Key != Off && this.SelectedPreset.Key != Custom;

        public bool ShowDeblockCustom => this.SelectedPreset != null && this.SelectedPreset.Key == Custom;

        public FilterPreset SelectedPreset
        {
            get => this.CurrentTask.DeblockPreset;

            set
            {
                if (Equals(value, this.CurrentTask.DeblockPreset)) return;
                this.CurrentTask.DeblockPreset = value;

                this.NotifyOfPropertyChange(() => this.SelectedPreset);
                this.NotifyOfPropertyChange(() => this.ShowDeblockTune);
                this.NotifyOfPropertyChange(() => this.ShowDeblockCustom);
                this.triggerTabChanged();
            }
        }

        public FilterTune SelectedTune
        {
            get => this.CurrentTask.DeblockTune;

            set
            {
                if (Equals(value, this.CurrentTask.DeblockTune)) return;
                this.CurrentTask.DeblockTune = value;

                this.NotifyOfPropertyChange(() => this.SelectedTune);
                this.triggerTabChanged();
            }
        }

        public string CustomDeblock
        {
            get => this.CurrentTask.CustomDeblock;

            set
            {
                if (value == this.CurrentTask.CustomDeblock) return;
                this.CurrentTask.CustomDeblock = value;
                this.NotifyOfPropertyChange(() => this.CustomDeblock);
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DEBLOCK).FirstOrDefault(s => s.ShortName == "off"));
                this.CustomDeblock = string.Empty;
                this.SelectedTune = null;
                return;
            }

            this.SelectedPreset = preset.Task.DeblockPreset;
            this.SelectedTune = preset.Task.DeblockTune;
            this.CustomDeblock = preset.Task.CustomDeblock;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.SelectedPreset);
            this.NotifyOfPropertyChange(() => this.SelectedTune);
            this.NotifyOfPropertyChange(() => this.CustomDeblock);

            this.NotifyOfPropertyChange(() => this.ShowDeblockTune);
            this.NotifyOfPropertyChange(() => this.ShowDeblockCustom);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (this.SelectedPreset?.Key != preset.Task?.DeblockPreset?.Key)
            {
                return false;
            }

            if (this.SelectedTune.Key != preset?.Task?.DeblockTune.Key)
            {
                return false;
            }

            if (this.CustomDeblock != preset?.Task?.CustomDeblock)
            {
                return false;
            }

            return true;
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }

        private void SetPresets()
        {
            BindingList<FilterPreset> presets = new BindingList<FilterPreset>();
            foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DEBLOCK))
            {
                presets.Add(new FilterPreset(tune));
            }

            this.Presets = presets;
            this.NotifyOfPropertyChange(() => this.Presets);
        }

        private void SetTunes()
        {
            BindingList<FilterTune> tunes = new BindingList<FilterTune>();
            foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_DEBLOCK))
            {
                tunes.Add(new FilterTune(tune));
            }

            this.Tunes = tunes;
            this.NotifyOfPropertyChange(() => this.Tunes);
        }
    }
}
