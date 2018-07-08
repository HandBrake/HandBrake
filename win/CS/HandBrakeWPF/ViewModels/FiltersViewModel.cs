// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FiltersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Filters View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using DenoisePreset = HandBrakeWPF.Services.Encode.Model.Models.DenoisePreset;
    using DenoiseTune = HandBrakeWPF.Services.Encode.Model.Models.DenoiseTune;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Filters View Model
    /// </summary>
    public class FiltersViewModel : ViewModelBase, IFiltersViewModel
    {
        private DeinterlaceFilter deinterlaceFilter;

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="FiltersViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public FiltersViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.CurrentTask = new EncodeTask();
            this.DeblockValue = 4; // OFF
            this.SelectedDeinterlaceFilter = DeinterlaceFilter.Off;
        }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets CurrentTask.
        /// </summary>
        public EncodeTask CurrentTask { get; private set; }

        /// <summary>
        /// Gets or sets CustomDenoise.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets CustomDetelecine.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets DeblockText.
        /// </summary>
        public string DeblockText
        {
            get
            {
                return this.DeblockValue == 4 ? "Off" : this.DeblockValue.ToString(CultureInfo.InvariantCulture);
            }
        }

        /// <summary>
        /// Gets or sets DeblockValue.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets DenoiseOptions.
        /// </summary>
        public IEnumerable<Denoise> DenoiseOptions
        {
            get
            {
                return EnumHelper<Denoise>.GetEnumList();
            }
        }

        /// <summary>
        /// Gets DetelecineOptions.
        /// </summary>
        public IEnumerable<Detelecine> DetelecineOptions
        {
            get
            {
                return EnumHelper<Detelecine>.GetEnumList();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether Grayscale.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        #region Interlace Detection

        /// <summary>
        /// Comb Detection Presets
        /// </summary>
        public IEnumerable<CombDetect> CombDetectPresets
        {
            get
            {
                return EnumHelper<CombDetect>.GetEnumList();
            }
        }

        /// <summary>
        /// Show the CombDetect Custom Box.
        /// </summary>
        public bool ShowCombDetectCustom => this.SelectedCombDetectPreset == CombDetect.Custom;

        /// <summary>
        /// Gets or sets the selected comb detect preset.
        /// </summary>
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
                this.OnTabStatusChanged(null);
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
                this.OnTabStatusChanged(null);
            }
        }

        #endregion

        #region Deinterlace and Decomb

        public IEnumerable<DeinterlaceFilter> DeinterlaceFilterOptions => EnumHelper<DeinterlaceFilter>.GetEnumList();

        public DeinterlaceFilter SelectedDeinterlaceFilter
        {
            get
            {
                return this.deinterlaceFilter;
            }

            set
            {
                if (value == this.deinterlaceFilter)
                {
                    return;
                }

                this.deinterlaceFilter = value;
                this.CurrentTask.DeinterlaceFilter = value;

                this.NotifyOfPropertyChange(() => this.SelectedDeinterlaceFilter);
                this.NotifyOfPropertyChange(() => this.ShowCustomDeinterlace);
                this.NotifyOfPropertyChange(() => this.DeinterlacePresets);
                this.NotifyOfPropertyChange(() => this.IsDeinterlaceEnabled);

                if (!this.DeinterlacePresets.Contains(this.SelectedDeInterlacePreset))
                {
                    this.SelectedDeInterlacePreset = this.DeinterlacePresets.FirstOrDefault(p => p.ShortName == "default");
                }
               
                this.OnTabStatusChanged(null);
            }
        }

        public IEnumerable<HBPresetTune> DeinterlacePresets
        {
            get
            {
                switch (this.SelectedDeinterlaceFilter)
                {
                    case DeinterlaceFilter.Yadif:
                        return new BindingList<HBPresetTune>(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DEINTERLACE));
                    case DeinterlaceFilter.Decomb:
                        return new BindingList<HBPresetTune>(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_DECOMB));
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
                this.OnTabStatusChanged(null);
            }
        }
        
        public bool IsDeinterlaceEnabled => this.CurrentTask.DeinterlaceFilter != DeinterlaceFilter.Off;

        public bool ShowCustomDeinterlace => this.IsDeinterlaceEnabled && this.CurrentTask.DeinterlacePreset?.ShortName == "custom";

        /// <summary>
        /// Gets or sets CustomDecomb.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        #endregion

        /// <summary>
        /// Gets or sets SelectedDetelecine.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDetelecineCustom.
        /// </summary>
        public bool ShowDetelecineCustom => this.CurrentTask.Detelecine == Detelecine.Custom;


        #region Denoise

        /// <summary>
        /// Gets or sets SelectedDenoise.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the selected denoise tune.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets or sets the selected denoise preset.
        /// </summary>
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
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Gets the denoise presets.
        /// </summary>
        public IEnumerable<DenoisePreset> DenoisePresets => EnumHelper<DenoisePreset>.GetEnumList();

        /// <summary>
        /// Gets the denoise tunes.
        /// </summary>
        public IEnumerable<DenoiseTune> DenoiseTunes => EnumHelper<DenoiseTune>.GetEnumList();

        public bool ShowDenoiseOptions => this.SelectedDenoise != Denoise.Off;

        public bool ShowDenoiseTune => this.SelectedDenoise == Denoise.NLMeans && this.SelectedDenoisePreset != DenoisePreset.Custom;

        public bool ShowDenoiseCustom => this.CurrentTask.DenoisePreset == DenoisePreset.Custom;

        #endregion

        /// <summary>
        /// The rotation options.
        /// </summary>
        public BindingList<int> RotationOptions => new BindingList<int> { 0, 90, 180, 270 };

        /// <summary>
        /// Selected Rotation.
        /// </summary>
        public int SelectedRotation
        {
            get
            {
                return this.CurrentTask.Rotation;
            }

            set
            {
                this.CurrentTask.Rotation = value;
                this.NotifyOfPropertyChange(() => this.SelectedRotation);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Flip the Video
        /// </summary>
        public bool FlipVideo
        {
            get
            {
                return this.CurrentTask.FlipVideo;
            }

            set
            {
                this.CurrentTask.FlipVideo = value;
                this.NotifyOfPropertyChange(() => this.FlipVideo);
                this.OnTabStatusChanged(null);
            }
        }

        #region Sharpen Filter

        public IEnumerable<Sharpen> SharpenOptions
        {
            get
            {
                return EnumHelper<Sharpen>.GetEnumList();
            }
        }

        public object SharpenPresets { get; set; }

        public object SharpenTunes { get; set; }

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
                            this.SelectedSharpenPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_LAPSHARP).FirstOrDefault(s => s.ShortName == "medium"));
                        if (this.SelectedSharpenTune == null)
                            this.SelectedSharpenTune = new FilterTune(HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_LAPSHARP).FirstOrDefault(s => s.ShortName == "none"));
                        break;
                    case Sharpen.UnSharp:
                        if (this.SelectedSharpenPreset == null)
                            this.SelectedSharpenPreset = new FilterPreset(HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_UNSHARP).FirstOrDefault(s => s.ShortName == "medium"));
                        if (this.SelectedSharpenTune == null)
                            this.SelectedSharpenTune = new FilterTune(HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_UNSHARP).FirstOrDefault(s => s.ShortName == "none"));
                        break;
                }

                this.NotifyOfPropertyChange(() => this.SelectedSharpenTune);
                this.NotifyOfPropertyChange(() => this.SelectedSharpenPreset);
                this.OnTabStatusChanged(null);
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
                this.OnTabStatusChanged(null);
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
                this.OnTabStatusChanged(null);
            }
        }

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
                this.OnTabStatusChanged(null);
            }
        }

        public bool ShowSharpenTune => this.SelectedSharpenPreset != null && this.SelectedSharpenPreset.DisplayName != "Custom";

        public bool ShowSharpenCustom => this.SelectedSharpenPreset != null && this.SelectedSharpenPreset.DisplayName == "Custom";

        public bool ShowSharpenOptions => this.SelectedSharpen != Sharpen.Off;

        #endregion

        #endregion

        #region Implemented Interfaces

        /// <summary>
        /// Setup this tab for the specified preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;

            if (preset != null)
            {
                // Properties
                this.SelectedDenoise = preset.Task.Denoise;
                this.SelectedDetelecine = preset.Task.Detelecine;

                this.SelectedDeinterlaceFilter = preset.Task.DeinterlaceFilter;
                this.SelectedDeInterlacePreset = preset.Task.DeinterlacePreset;

                this.SelectedCombDetectPreset = preset.Task.CombDetect;

                this.Grayscale = preset.Task.Grayscale;
                this.DeblockValue = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;
                this.SelectedDenoisePreset = preset.Task.DenoisePreset;
                this.SelectedDenoiseTune = preset.Task.DenoiseTune;

                // Sharpen
                this.SelectedSharpen = preset.Task.Sharpen;
                this.SelectedSharpenPreset = preset.Task.SharpenPreset;
                this.SelectedSharpenTune = preset.Task.SharpenTune;
                this.CustomSharpen = preset.Task.SharpenCustom;

                // Custom Values
                this.CustomDeinterlaceSettings = preset.Task.CustomDeinterlaceSettings;
                this.CustomCombDetect = preset.Task.CustomCombDetect;
                this.CustomDetelecine = preset.Task.CustomDetelecine;
                this.CustomDenoise = preset.Task.CustomDenoise;

                this.SelectedRotation = preset.Task.Rotation;
                this.FlipVideo = preset.Task.FlipVideo;
            }
            else
            {
                // Default everything to off
                this.SelectedDenoise = Denoise.Off;
                this.SelectedDeinterlaceFilter = DeinterlaceFilter.Off;
                this.SelectedDetelecine = Detelecine.Off;
                this.Grayscale = false;
                this.DeblockValue = 0;

                this.SelectedRotation = 0;
                this.FlipVideo = false;
            }
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask task)
        {
            this.CurrentTask = task;

            this.NotifyOfPropertyChange(() => this.SelectedDenoise);
            this.NotifyOfPropertyChange(() => this.SelectedDeinterlaceFilter);
            this.NotifyOfPropertyChange(() => this.SelectedDeInterlacePreset);
            this.NotifyOfPropertyChange(() => this.SelectedDetelecine);
            this.NotifyOfPropertyChange(() => this.Grayscale);
            this.NotifyOfPropertyChange(() => this.DeblockValue);
            this.NotifyOfPropertyChange(() => this.SelectedSharpen);
            this.NotifyOfPropertyChange(() => this.SelectedSharpenPreset);
            this.NotifyOfPropertyChange(() => this.SelectedSharpenTune);
            this.NotifyOfPropertyChange(() => this.SelectedCombDetectPreset);
            this.NotifyOfPropertyChange(() => this.SelectedDenoisePreset);
            this.NotifyOfPropertyChange(() => this.SelectedDenoiseTune);
            this.NotifyOfPropertyChange(() => this.FlipVideo);
            this.NotifyOfPropertyChange(() => this.SelectedRotation);

            this.NotifyOfPropertyChange(() => this.CustomDeinterlaceSettings);
            this.NotifyOfPropertyChange(() => this.CustomDetelecine);
            this.NotifyOfPropertyChange(() => this.CustomDenoise);
            this.NotifyOfPropertyChange(() => this.CustomSharpen);
            this.NotifyOfPropertyChange(() => this.CustomCombDetect);


            this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
            this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);
            this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
            this.NotifyOfPropertyChange(() => this.ShowCustomDeinterlace);
            this.NotifyOfPropertyChange(() => this.ShowCombDetectCustom);
            this.NotifyOfPropertyChange(() => this.ShowDetelecineCustom);       
            this.NotifyOfPropertyChange(() => this.ShowSharpenCustom);
            this.NotifyOfPropertyChange(() => this.ShowSharpenOptions);
            this.NotifyOfPropertyChange(() => this.ShowSharpenTune);
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

            int presetDeblock = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;

            if (presetDeblock != this.DeblockValue)
            {
                return false;
            }

            if (preset.Task.Grayscale != this.Grayscale)
            {
                return false;
            }

            if (preset.Task.Rotation != this.SelectedRotation)
            {
                return false;
            }

            if (preset.Task.FlipVideo != this.FlipVideo)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.CurrentTask = task;
        }

        #endregion

        #region Private Methods
        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }
        #endregion
    }
}