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
    using System.Collections.Generic;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using Action = System.Action;

    public class DenoiseItem : PropertyChangedBase
    {
        private readonly Action triggerTabChanged;

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

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);

                this.SelectedDenoisePreset = this.CurrentTask.Denoise == Denoise.hqdn3d ? DenoisePreset.Weak : DenoisePreset.Ultralight; // Default so we don't have an invalid preset.

                this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
                this.triggerTabChanged();
            }
        }

        public DenoiseTune SelectedDenoiseTune
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

        public DenoisePreset SelectedDenoisePreset
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
                if (value != DenoisePreset.Custom) this.CustomDenoise = string.Empty;
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

        public IEnumerable<DenoisePreset> DenoisePresets => EnumHelper<DenoisePreset>.GetEnumList();

        public IEnumerable<DenoiseTune> DenoiseTunes => EnumHelper<DenoiseTune>.GetEnumList();

        public IEnumerable<Denoise> DenoiseOptions => EnumHelper<Denoise>.GetEnumList();

        public bool ShowDenoiseOptions => this.SelectedDenoise != Denoise.Off;

        public bool ShowDenoiseTune => this.SelectedDenoise == Denoise.NLMeans && this.SelectedDenoisePreset != DenoisePreset.Custom;

        public bool ShowDenoiseCustom => this.CurrentTask.DenoisePreset == DenoisePreset.Custom;

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

            if (this.SelectedDenoise != Denoise.Off && preset.Task.DenoisePreset != this.SelectedDenoisePreset)
            {
                return false;
            }

            if (this.SelectedDenoise != Denoise.Off && preset.Task.DenoiseTune != this.SelectedDenoiseTune)
            {
                return false;
            }

            return true;
        }
    }
}
