// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SharpenItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SharpenItem type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.Collections.Generic;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Action = System.Action;

    public class SharpenItem : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

        public SharpenItem(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
        }

        public EncodeTask CurrentTask { get; private set; }

        public Sharpen SelectedSharpen
        {
            get
            {
                return this.CurrentTask.Sharpen;
            }

            set
            {
                if (value == this.CurrentTask.Sharpen) return;
                this.CurrentTask.Sharpen = value;
                this.NotifyOfPropertyChange(() => this.SelectedSharpen);
                this.NotifyOfPropertyChange(() => this.ShowSharpenOptions);
                this.NotifyOfPropertyChange(() => this.ShowSharpenTune);
                this.NotifyOfPropertyChange(() => this.ShowSharpenCustom);

                // Default preset and tune.
                switch (value)
                {
                    case Sharpen.LapSharp:
                        if (this.SelectedSharpenPreset == null)
                            this.SelectedSharpenPreset = new FilterPreset(
                                HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_LAPSHARP)
                                    .FirstOrDefault(s => s.ShortName == "medium"));
                        if (this.SelectedSharpenTune == null)
                            this.SelectedSharpenTune = new FilterTune(
                                HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_LAPSHARP)
                                    .FirstOrDefault(s => s.ShortName == "none"));
                        break;
                    case Sharpen.UnSharp:
                        if (this.SelectedSharpenPreset == null)
                            this.SelectedSharpenPreset = new FilterPreset(
                                HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_UNSHARP)
                                    .FirstOrDefault(s => s.ShortName == "medium"));
                        if (this.SelectedSharpenTune == null)
                            this.SelectedSharpenTune = new FilterTune(
                                HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_UNSHARP)
                                    .FirstOrDefault(s => s.ShortName == "none"));
                        break;
                }

                this.NotifyOfPropertyChange(() => this.SelectedSharpenTune);
                this.NotifyOfPropertyChange(() => this.SelectedSharpenPreset);
                this.triggerTabChanged();
            }
        }

        public FilterPreset SelectedSharpenPreset
        {
            get
            {
                return this.CurrentTask.SharpenPreset;
            }

            set
            {
                if (Equals(value, this.CurrentTask.SharpenPreset)) return;
                this.CurrentTask.SharpenPreset = value;
                this.NotifyOfPropertyChange(() => this.SelectedSharpenPreset);
                this.NotifyOfPropertyChange(() => this.ShowSharpenTune);
                this.NotifyOfPropertyChange(() => this.ShowSharpenCustom);
                this.triggerTabChanged();
            }
        }

        public FilterTune SelectedSharpenTune
        {
            get
            {
                return this.CurrentTask.SharpenTune;
            }

            set
            {
                if (value == this.CurrentTask.SharpenTune) return;
                this.CurrentTask.SharpenTune = value;
                this.NotifyOfPropertyChange(() => this.SelectedSharpenTune);
                this.triggerTabChanged();
            }
        }

        public IEnumerable<Sharpen> SharpenOptions
        {
            get
            {
                return EnumHelper<Sharpen>.GetEnumList();
            }
        }

        public object SharpenPresets { get; set; }

        public object SharpenTunes { get; set; }

        public string CustomSharpen
        {
            get
            {
                return this.CurrentTask.SharpenCustom;
            }

            set
            {
                if (value == this.CurrentTask.SharpenCustom) return;
                this.CurrentTask.SharpenCustom = value;
                this.NotifyOfPropertyChange(() => this.CustomSharpen);
                this.triggerTabChanged();
            }
        }

        public bool ShowSharpenTune =>
            this.SelectedSharpenPreset != null && this.SelectedSharpenPreset.DisplayName != "Custom";

        public bool ShowSharpenCustom =>
            this.SelectedSharpenPreset != null && this.SelectedSharpenPreset.DisplayName == "Custom";

        public bool ShowSharpenOptions => this.SelectedSharpen != Sharpen.Off;

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedSharpen = Sharpen.Off;
                return;
            }

            this.SelectedSharpen = preset.Task.Sharpen;
            this.SelectedSharpenPreset = preset.Task.SharpenPreset;
            this.SelectedSharpenTune = preset.Task.SharpenTune;
            this.CustomSharpen = preset.Task.SharpenCustom;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.SelectedSharpen);
            this.NotifyOfPropertyChange(() => this.SelectedSharpenPreset);
            this.NotifyOfPropertyChange(() => this.SelectedSharpenTune);
            this.NotifyOfPropertyChange(() => this.CustomSharpen);
            this.NotifyOfPropertyChange(() => this.ShowSharpenCustom);
            this.NotifyOfPropertyChange(() => this.ShowSharpenOptions);
            this.NotifyOfPropertyChange(() => this.ShowSharpenTune);
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.Sharpen != this.SelectedSharpen)
            {
                return false;
            }

            if (this.SelectedSharpen != Sharpen.Off && !Equals(preset.Task.SharpenPreset, this.SelectedSharpenPreset))
            {
                return false;
            }

            if (this.SelectedSharpen != Sharpen.Off && !Equals(preset.Task.SharpenTune, this.SelectedSharpenTune))
            {
                return false;
            }

            return true;
        }
    }
}
