// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DeinterlaceFilterItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DeinterlaceFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Linq;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

    using Action = System.Action;

    public class DeinterlaceFilterItem : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

        public DeinterlaceFilterItem(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
            this.SelectedDeinterlaceFilter = DeinterlaceFilter.Off;
        }

        public EncodeTask CurrentTask { get; private set; }

        public IEnumerable<DeinterlaceFilter> DeinterlaceFilterOptions => EnumHelper<DeinterlaceFilter>.GetEnumList();

        public IEnumerable<CombDetect> CombDetectPresets => EnumHelper<CombDetect>.GetEnumList();

        public bool IsDeinterlaceEnabled => this.CurrentTask.DeinterlaceFilter != DeinterlaceFilter.Off;

        public bool ShowCustomDeinterlace => this.IsDeinterlaceEnabled && this.CurrentTask.DeinterlacePreset?.ShortName == "custom";

        public bool ShowCombDetectCustom => this.SelectedCombDetectPreset == CombDetect.Custom;
        
        public DeinterlaceFilter SelectedDeinterlaceFilter
        {
            get
            {
                return this.CurrentTask.DeinterlaceFilter;
            }

            set
            {
                if (value == this.CurrentTask.DeinterlaceFilter)
                {
                    return;
                }

                this.CurrentTask.DeinterlaceFilter = value;

                this.NotifyOfPropertyChange(() => this.SelectedDeinterlaceFilter);
                this.NotifyOfPropertyChange(() => this.ShowCustomDeinterlace);
                this.NotifyOfPropertyChange(() => this.DeinterlacePresets);
                this.NotifyOfPropertyChange(() => this.IsDeinterlaceEnabled);

                if (!this.DeinterlacePresets.Contains(this.SelectedDeInterlacePreset))
                {
                    this.SelectedDeInterlacePreset = this.DeinterlacePresets.FirstOrDefault(p => p.ShortName == "default");
                }

                this.triggerTabChanged();
            }
        }

        public IEnumerable<HBPresetTune> DeinterlacePresets
        {
            get
            {
                switch (this.SelectedDeinterlaceFilter)
                {
                    case DeinterlaceFilter.Yadif:
                        return new BindingList<HBPresetTune>(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_YADIF));
                    case DeinterlaceFilter.Decomb:
                        return new BindingList<HBPresetTune>(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DECOMB));
                    case DeinterlaceFilter.Bwdif:
                        return new BindingList<HBPresetTune>(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_BWDIF));
                    default:
                        return new BindingList<HBPresetTune>();
                }
            }
        }

        public HBPresetTune SelectedDeInterlacePreset
        {
            get
            {
                return this.CurrentTask.DeinterlacePreset;
            }

            set
            {
                this.CurrentTask.DeinterlacePreset = value;
                this.NotifyOfPropertyChange(() => this.SelectedDeInterlacePreset);

                if (value?.ShortName == "custom") this.CustomDeinterlaceSettings = string.Empty;

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowCustomDeinterlace);
                this.triggerTabChanged();
            }
        }

        public CombDetect SelectedCombDetectPreset
        {
            get
            {
                return this.CurrentTask.CombDetect;
            }

            set
            {
                this.CurrentTask.CombDetect = value;
                this.NotifyOfPropertyChange(() => this.SelectedCombDetectPreset);

                if (value != CombDetect.Custom) this.CustomCombDetect = string.Empty;

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowCombDetectCustom);
                this.triggerTabChanged();
            }
        }

        /// <summary>
        /// Gets or sets the custom comb detect.
        /// </summary>
        public string CustomCombDetect
        {
            get
            {
                return this.CurrentTask.CustomCombDetect;
            }

            set
            {
                this.CurrentTask.CustomCombDetect = value;
                this.NotifyOfPropertyChange(() => this.CustomCombDetect);
                this.triggerTabChanged();
            }
        }

        public string CustomDeinterlaceSettings
        {
            get
            {
                return this.CurrentTask.CustomDeinterlaceSettings;
            }

            set
            {
                this.CurrentTask.CustomDeinterlaceSettings = value;
                this.NotifyOfPropertyChange(() => this.CustomDeinterlaceSettings);
                this.triggerTabChanged();
            }
        }

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedDeinterlaceFilter = DeinterlaceFilter.Off;
                return;
            }

            this.SelectedDeinterlaceFilter = preset.Task.DeinterlaceFilter;
            this.SelectedDeInterlacePreset = preset.Task.DeinterlacePreset;
            this.CustomDeinterlaceSettings = preset.Task.CustomDeinterlaceSettings;
            this.SelectedCombDetectPreset = preset.Task.CombDetect;
            this.CustomCombDetect = preset.Task.CustomCombDetect;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;
            this.NotifyOfPropertyChange(() => this.SelectedDeinterlaceFilter);
            this.NotifyOfPropertyChange(() => this.SelectedDeInterlacePreset);
            this.NotifyOfPropertyChange(() => this.CustomDeinterlaceSettings);
            this.NotifyOfPropertyChange(() => this.ShowCustomDeinterlace);
            this.NotifyOfPropertyChange(() => this.CustomCombDetect);
            this.NotifyOfPropertyChange(() => this.SelectedCombDetectPreset);
            this.NotifyOfPropertyChange(() => this.ShowCombDetectCustom);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.DeinterlaceFilter != this.SelectedDeinterlaceFilter)
            {
                return false;
            }

            if (preset.Task.DeinterlacePreset != this.SelectedDeInterlacePreset)
            {
                return false;
            }

            if (preset.Task.CustomDeinterlaceSettings != this.CustomDeinterlaceSettings)
            {
                return false;
            }

            if (preset.Task.CombDetect != this.SelectedCombDetectPreset)
            {
                return false;
            }

            if ((preset.Task.CustomCombDetect ?? string.Empty) != (this.CustomCombDetect ?? string.Empty))
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
