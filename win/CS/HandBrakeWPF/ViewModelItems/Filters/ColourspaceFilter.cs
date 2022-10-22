// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ColourSpaceFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ColourSpaceFilter type.
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

    public class ColourSpaceFilter : PropertyChangedBase
    {
        public static readonly string Off = "off";
        public static readonly string Custom = "custom";

        private readonly Action triggerTabChanged;

        public ColourSpaceFilter(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;

            this.SetPresets();
        }

        public EncodeTask CurrentTask { get; private set; }

        public object Presets { get; set; }

        public bool ShowColourspaceCustom => this.SelectedPreset != null && this.SelectedPreset.Key == Custom;

        public FilterPreset SelectedPreset
        {
            get => this.CurrentTask.Colourspace;

            set
            {
                if (Equals(value, this.CurrentTask.Colourspace))
                {
                    return;
                }

                this.CurrentTask.Colourspace = value;

                this.NotifyOfPropertyChange(() => this.SelectedPreset);
                this.NotifyOfPropertyChange(() => this.ShowColourspaceCustom);
                this.NotifyOfPropertyChange(() => this.ShowColourspaceCustom);
                this.triggerTabChanged();
            }
        }

        public string CustomColourspace
        {
            get => this.CurrentTask.CustomColourspace;

            set
            {
                if (value == this.CurrentTask.CustomColourspace)
                {
                    return;
                }

                this.CurrentTask.CustomColourspace = value;
                this.NotifyOfPropertyChange(() => this.CustomColourspace);
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_COLORSPACE).FirstOrDefault(s => s.ShortName == "off"));
                this.CustomColourspace = string.Empty;
                return;
            }

            this.SelectedPreset = preset.Task.Colourspace;
            this.CustomColourspace = preset.Task.CustomColourspace;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.SelectedPreset);
            this.NotifyOfPropertyChange(() => this.CustomColourspace);

            this.NotifyOfPropertyChange(() => this.ShowColourspaceCustom);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (this.SelectedPreset?.Key != preset.Task?.Colourspace?.Key)
            {
                return false;
            }

            if (this.CustomColourspace != preset?.Task?.CustomColourspace)
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
            foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_COLORSPACE))
            {
                presets.Add(new FilterPreset(tune));
            }

            this.Presets = presets;
            this.NotifyOfPropertyChange(() => this.Presets);
        }
    }
}
