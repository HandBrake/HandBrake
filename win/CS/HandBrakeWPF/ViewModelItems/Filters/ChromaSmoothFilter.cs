// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChromaSmoothFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Chroma Smooth Filter type.
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

    public class ChromaSmoothFilter : PropertyChangedBase
    {
        public static readonly string Off = "off";
        public static readonly string Custom = "custom";

        private readonly Action triggerTabChanged;

        public ChromaSmoothFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;

            this.SetPresets();
            this.SetTunes();
        }

        public EncodeTask CurrentTask { get; private set; }

        public BindingList<FilterPreset> Presets { get; set; }

        public BindingList<FilterTune> Tunes { get; set; }

        public bool ShowTune => this.SelectedPreset != null && this.SelectedPreset.Key != Off && this.SelectedPreset.Key != Custom;

        public bool ShowCustom => this.SelectedPreset != null && this.SelectedPreset.Key == Custom;

        public FilterPreset SelectedPreset
        {
            get => this.CurrentTask.ChromaSmooth;

            set
            {
                if (Equals(value, this.CurrentTask.ChromaSmooth))
                {
                    return;
                }

                this.CurrentTask.ChromaSmooth = value;

                this.NotifyOfPropertyChange(() => this.SelectedPreset);
                this.NotifyOfPropertyChange(() => this.ShowTune);
                this.NotifyOfPropertyChange(() => this.ShowCustom);

                if (this.SelectedTune == null)
                {
                    this.SelectedTune = this.Tunes.FirstOrDefault();
                }

                this.triggerTabChanged();
            }
        }

        public FilterTune SelectedTune
        {
            get => this.CurrentTask.ChromaSmoothTune;

            set
            {
                if (Equals(value, this.CurrentTask.ChromaSmoothTune))
                {
                    return;
                }

                this.CurrentTask.ChromaSmoothTune = value;

                this.NotifyOfPropertyChange(() => this.SelectedTune);
                this.triggerTabChanged();
            }
        }

        public string CustomSettings
        {
            get => this.CurrentTask.CustomChromaSmooth;

            set
            {
                if (value == this.CurrentTask.CustomChromaSmooth)
                {
                    return;
                }

                this.CurrentTask.CustomChromaSmooth = value;
                this.NotifyOfPropertyChange(() => this.CustomSettings);
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_CHROMA_SMOOTH).FirstOrDefault(s => s.ShortName == "off"));
                this.CustomSettings = string.Empty;
                this.SelectedTune = null;
                return;
            }

            this.SelectedPreset = preset.Task.ChromaSmooth;
            this.SelectedTune = preset.Task.ChromaSmoothTune;
            this.CustomSettings = preset.Task.CustomChromaSmooth;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.SelectedPreset);
            this.NotifyOfPropertyChange(() => this.SelectedTune);
            this.NotifyOfPropertyChange(() => this.CustomSettings);

            this.NotifyOfPropertyChange(() => this.ShowTune);
            this.NotifyOfPropertyChange(() => this.ShowCustom);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (this.SelectedPreset?.Key != preset.Task?.ChromaSmooth?.Key)
            {
                return false;
            }

            if (this.SelectedTune?.Key != preset?.Task?.ChromaSmoothTune?.Key)
            {
                return false;
            }

            if (this.CustomSettings != preset?.Task?.CustomChromaSmooth)
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
            foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_CHROMA_SMOOTH))
            {
                presets.Add(new FilterPreset(tune));
            }

            this.Presets = presets;
            this.NotifyOfPropertyChange(() => this.Presets);
        }

        private void SetTunes()
        {
            BindingList<FilterTune> tunes = new BindingList<FilterTune>();
            foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_CHROMA_SMOOTH))
            {
                tunes.Add(new FilterTune(tune));
            }

            this.Tunes = tunes;
            this.NotifyOfPropertyChange(() => this.Tunes);
        }
    }
}
