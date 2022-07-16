// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoiseItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoiseItem type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModelItems.Filters
{
    using System;
    using System.Collections.Generic;
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

    public class DenoiseItem : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;
        private int currentFilterId;

        public DenoiseItem(EncodeTask currentTask, Action triggerTabChanged)
        {
            this.triggerTabChanged = triggerTabChanged;
            this.CurrentTask = currentTask;
        }

        public EncodeTask CurrentTask { get; private set; }

        public Denoise SelectedDenoise
        {
            get
            {
                return this.CurrentTask.Denoise;
            }

            set
            {
                this.CurrentTask.Denoise = value;
                this.NotifyOfPropertyChange(() => this.SelectedDenoise);

                this.currentFilterId = this.CurrentTask.Denoise switch
                {
                    Denoise.NLMeans => (int)hb_filter_ids.HB_FILTER_NLMEANS,
                    Denoise.hqdn3d => (int)hb_filter_ids.HB_FILTER_HQDN3D,
                    Denoise.Off => 0,
                    _ => throw new ArgumentOutOfRangeException()
                };

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);

                this.NotifyOfPropertyChange(() => this.DenoisePresets);
                this.NotifyOfPropertyChange(() => this.DenoiseTunes);
                
                this.SelectedDenoisePreset = this.DenoisePresets.FirstOrDefault(s => s.ShortName != HBPresetTune.Custom); // Default so we don't have an invalid preset.
                this.SelectedDenoiseTune = this.DenoiseTunes?.FirstOrDefault(s => s.ShortName == HBPresetTune.None); // Default so we don't have an invalid tune.
                    
                this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
                this.triggerTabChanged();
            }
        }

        public HBPresetTune SelectedDenoiseTune
        {
            get
            {
                return this.CurrentTask.DenoiseTune;
            }

            set
            {
                this.CurrentTask.DenoiseTune = value;
                this.NotifyOfPropertyChange(() => this.SelectedDenoiseTune);
                this.triggerTabChanged();
            }
        }

        public HBPresetTune SelectedDenoisePreset
        {
            get
            {
                return this.CurrentTask.DenoisePreset;
            }

            set
            {
                this.CurrentTask.DenoisePreset = value;
                this.NotifyOfPropertyChange(() => this.SelectedDenoisePreset);

                // Show / Hide the Custom Control
                if (value?.ShortName != HBPresetTune.Custom) this.CustomDenoise = string.Empty;
                this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
                this.triggerTabChanged();
            }
        }

        public string CustomDenoise
        {
            get
            {
                return this.CurrentTask.CustomDenoise;
            }

            set
            {
                this.CurrentTask.CustomDenoise = value;
                this.NotifyOfPropertyChange(() => this.CustomDenoise);
                this.triggerTabChanged();
            }
        }

        public IEnumerable<HBPresetTune> DenoisePresets => HandBrakeFilterHelpers.GetFilterPresets(currentFilterId);

        public IEnumerable<HBPresetTune> DenoiseTunes => HandBrakeFilterHelpers.GetFilterTunes(currentFilterId);

        public IEnumerable<Denoise> DenoiseOptions => EnumHelper<Denoise>.GetEnumList();

        public bool ShowDenoiseOptions => this.SelectedDenoise != Denoise.Off;

        public bool ShowDenoiseTune => this.SelectedDenoise == Denoise.NLMeans && this.SelectedDenoisePreset?.ShortName != HBPresetTune.Custom;

        public bool ShowDenoiseCustom => this.CurrentTask.DenoisePreset?.ShortName == HBPresetTune.Custom;

        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset == null)
            {
                this.SelectedDenoise = Denoise.Off;        
                return;
            }

            this.SelectedDenoise = preset.Task.Denoise;
            this.SelectedDenoisePreset = preset.Task.DenoisePreset;
            this.SelectedDenoiseTune = preset.Task.DenoiseTune;
            this.CustomDenoise = preset.Task.CustomDenoise;
        }

        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;

            this.NotifyOfPropertyChange(() => this.SelectedDenoise);
            this.NotifyOfPropertyChange(() => this.SelectedDenoisePreset);
            this.NotifyOfPropertyChange(() => this.SelectedDenoiseTune);
            this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
            this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);
            this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
            this.NotifyOfPropertyChange(() => this.CustomDenoise);
        }

        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.Denoise != this.SelectedDenoise)
            {
                return false;
            }

            if (this.SelectedDenoise != Denoise.Off && !Equals(preset.Task.DenoisePreset, this.SelectedDenoisePreset))
            {
                return false;
            }

            if (this.SelectedDenoise != Denoise.Off && !Equals(preset.Task.DenoiseTune, this.SelectedDenoiseTune))
            {
                return false;
            }

            return true;
        }
    }
}
