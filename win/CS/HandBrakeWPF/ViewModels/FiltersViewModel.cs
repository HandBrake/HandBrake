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
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

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

        #region Properties

        /// <summary>
        /// Gets CurrentTask.
        /// </summary>
        public EncodeTask CurrentTask { get; private set; }

        /// <summary>
        /// Gets or sets CustomDecomb.
        /// </summary>
        public string CustomDecomb
        {
            get
            {
                return this.CurrentTask.CustomDecomb;
            }

            set
            {
                this.CurrentTask.CustomDecomb = value;
                this.NotifyOfPropertyChange(() => this.CustomDecomb);
            }
        }

        /// <summary>
        /// Gets or sets CustomDeinterlace.
        /// </summary>
        public string CustomDeinterlace
        {
            get
            {
                return this.CurrentTask.CustomDeinterlace;
            }

            set
            {
                this.CurrentTask.CustomDeinterlace = value;
                this.NotifyOfPropertyChange(() => this.CustomDeinterlace);
            }
        }

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
            }
        }

        /// <summary>
        /// Gets DeInterlaceOptions.
        /// </summary>
        public IEnumerable<Deinterlace> DeInterlaceOptions
        {
            get
            {
                return EnumHelper<Deinterlace>.GetEnumList();
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
            }
        }

        /// <summary>
        /// Gets DecombOptions.
        /// </summary>
        public IEnumerable<Decomb> DecombOptions
        {
            get
            {
                return EnumHelper<Decomb>.GetEnumList();
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
        /// Gets DeinterlaceFilterOptions.
        /// </summary>
        public IEnumerable<DeinterlaceFilter> DeinterlaceFilterOptions
        {
            get
            {
                return EnumHelper<DeinterlaceFilter>.GetEnumList();
            }
        }

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
            }
        }

        /// <summary>
        /// Gets or sets SelectedDeInterlace.
        /// </summary>
        public Deinterlace SelectedDeInterlace
        {
            get
            {
                return this.CurrentTask.Deinterlace;
            }

            set
            {
                this.CurrentTask.Deinterlace = value;
                this.NotifyOfPropertyChange(() => this.SelectedDeInterlace);

                if (value != Deinterlace.Custom) this.CustomDeinterlace = string.Empty;

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowDecombCustom);
                this.NotifyOfPropertyChange(() => this.ShowDeinterlaceCustom);
                this.NotifyOfPropertyChange(() => this.ShowDeinterlaceDecombCustom);
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

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowCombDetectCustom);
            }
        }

        /// <summary>
        /// Show the CombDetect Custom Box.
        /// </summary>
        public bool ShowCombDetectCustom
        {
            get
            {
                return this.SelectedCombDetectPreset == CombDetect.Custom;
            }
        }

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
            }
        }

        /// <summary>
        /// Gets or sets SelectedDecomb.
        /// </summary>
        public Decomb SelectedDecomb
        {
            get
            {
                return this.CurrentTask.Decomb;
            }

            set
            {
                this.CurrentTask.Decomb = value;
                this.NotifyOfPropertyChange(() => this.SelectedDecomb);

                if (value != Decomb.Custom) this.CustomDecomb = string.Empty;

                // Show / Hide the Custom Control
                this.NotifyOfPropertyChange(() => this.ShowDecombCustom);
                this.NotifyOfPropertyChange(() => this.ShowDeinterlaceCustom);
                this.NotifyOfPropertyChange(() => this.ShowDeinterlaceDecombCustom);
            }
        }

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
                this.ShowDenoiseCustom = this.CurrentTask.DenoisePreset == DenoisePreset.Custom;
                this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);

                this.SelectedDenoisePreset = this.CurrentTask.Denoise == Denoise.hqdn3d ? DenoisePreset.Weak : DenoisePreset.Ultralight; // Default so we don't have an invalid preset.

                this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
            }
        }

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
                this.ShowDetelecineCustom = this.CurrentTask.Detelecine == Detelecine.Custom;
                if (value != Detelecine.Custom) this.CustomDetelecine = string.Empty;
                this.NotifyOfPropertyChange(() => this.ShowDetelecineCustom);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDecombCustom.
        /// </summary>
        public bool ShowDecombCustom => this.SelectedDeinterlaceFilter == DeinterlaceFilter.Decomb && this.SelectedDecomb == Decomb.Custom;

        /// <summary>
        /// Gets or sets a value indicating whether ShowDeinterlaceDecombCustom.
        /// </summary>
        public bool ShowDeinterlaceDecombCustom => (this.SelectedDeinterlaceFilter == DeinterlaceFilter.Decomb && this.SelectedDecomb == Decomb.Custom) || (this.SelectedDeinterlaceFilter == DeinterlaceFilter.Yadif && this.SelectedDeInterlace == Deinterlace.Custom);

        /// <summary>
        /// Gets or sets a value indicating whether ShowDelelecineCustom.
        /// </summary>
        public bool ShowDeinterlaceCustom => this.SelectedDeinterlaceFilter == DeinterlaceFilter.Yadif && this.SelectedDeInterlace == Deinterlace.Custom;

        /// <summary>
        /// Gets or sets a value indicating whether ShowDenoiseCustom.
        /// </summary>
        public bool ShowDenoiseCustom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDetelecineCustom.
        /// </summary>
        public bool ShowDetelecineCustom { get; set; }

        /// <summary>
        /// Gets or sets the selected deinterlace filter mode.
        /// </summary>
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

                if (this.deinterlaceFilter == DeinterlaceFilter.Yadif)
                {
                    this.IsDeinterlaceMode = true;
                    this.IsDecombMode = false;
                }
                else if (this.deinterlaceFilter == DeinterlaceFilter.Decomb)
                {
                    this.IsDeinterlaceMode = false;
                    this.IsDecombMode = true;
                }
                else
                {
                    this.IsDeinterlaceMode = false;
                    this.IsDecombMode = false;
                }

                this.NotifyOfPropertyChange(() => this.SelectedDeinterlaceFilter);
                this.NotifyOfPropertyChange(() => this.IsDeinterlaceMode);
                this.NotifyOfPropertyChange(() => this.IsDecombMode);
                this.NotifyOfPropertyChange(() => this.IsDeinterlaceDecomb);
                this.NotifyOfPropertyChange(() => this.ShowDecombCustom);
                this.NotifyOfPropertyChange(() => this.ShowDeinterlaceCustom);
                this.NotifyOfPropertyChange(() => this.ShowDeinterlaceDecombCustom);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether is deinterlace mode.
        /// </summary>
        public bool IsDeinterlaceMode { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is decomb mode.
        /// </summary>
        public bool IsDecombMode { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is deinterlace or decomb mode.
        /// </summary>
        public bool IsDeinterlaceDecomb => this.SelectedDeinterlaceFilter != DeinterlaceFilter.Off;

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
                this.ShowDenoiseCustom = this.CurrentTask.DenoisePreset == DenoisePreset.Custom;
                if (value != DenoisePreset.Custom) this.CustomDenoise = string.Empty;
                this.NotifyOfPropertyChange(() => this.ShowDenoiseCustom);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseOptions);
                this.NotifyOfPropertyChange(() => this.ShowDenoiseTune);
            }
        }

        /// <summary>
        /// Gets the denoise presets.
        /// </summary>
        public IEnumerable<DenoisePreset> DenoisePresets
        {
            get
            {
                return EnumHelper<DenoisePreset>.GetEnumList();
            }
        }

        /// <summary>
        /// Gets the denoise tunes.
        /// </summary>
        public IEnumerable<DenoiseTune> DenoiseTunes
        {
            get
            {
                return EnumHelper<DenoiseTune>.GetEnumList();
            }
        }

        /// <summary>
        /// Gets a value indicating whether show denoise options.
        /// </summary>
        public bool ShowDenoiseOptions
        {
            get
            {
                return this.SelectedDenoise != Denoise.Off;
            }
        }

        /// <summary>
        /// Gets a value indicating whether show denoise tune.
        /// </summary>
        public bool ShowDenoiseTune
        {
            get
            {
                return this.SelectedDenoise == Denoise.NLMeans && this.SelectedDenoisePreset != DenoisePreset.Custom;
            }
        }

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
            }
        }

        #endregion

        #region Implemented Interfaces

        #region ITabInterface

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

                this.SelectedDecomb = preset.Task.Decomb;
                this.SelectedDeInterlace = preset.Task.Deinterlace;
                if (preset.Task.DeinterlaceFilter == DeinterlaceFilter.Yadif)
                {
                    this.SelectedDeinterlaceFilter = DeinterlaceFilter.Yadif;
                }
                else if (preset.Task.DeinterlaceFilter == DeinterlaceFilter.Decomb)
                {
                    this.SelectedDeinterlaceFilter = DeinterlaceFilter.Decomb;
                }
                else
                {
                    this.SelectedDeinterlaceFilter = DeinterlaceFilter.Off;
                }

                this.Grayscale = preset.Task.Grayscale;
                this.DeblockValue = preset.Task.Deblock == 0 ? 4 : preset.Task.Deblock;
                this.SelectedDenoisePreset = preset.Task.DenoisePreset;
                this.SelectedDenoiseTune = preset.Task.DenoiseTune;

                // Custom Values
                this.CustomDecomb = preset.Task.CustomDecomb;
                this.CustomDeinterlace = preset.Task.CustomDeinterlace;
                this.CustomDetelecine = preset.Task.CustomDetelecine;
                this.CustomDenoise = preset.Task.CustomDenoise;


                this.SelectedRotation = preset.Task.Rotation;
                this.FlipVideo = preset.Task.FlipVideo;
            }
            else
            {
                // Default everything to off
                this.SelectedDenoise = Denoise.Off;
                this.SelectedDecomb = Decomb.Default;
                this.SelectedDeInterlace = Deinterlace.Default;
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
            this.NotifyOfPropertyChange(() => this.SelectedDecomb);
            this.NotifyOfPropertyChange(() => this.SelectedDeInterlace);
            this.NotifyOfPropertyChange(() => this.SelectedDetelecine);
            this.NotifyOfPropertyChange(() => this.Grayscale);
            this.NotifyOfPropertyChange(() => this.DeblockValue);

            this.NotifyOfPropertyChange(() => this.CustomDecomb);
            this.NotifyOfPropertyChange(() => this.CustomDeinterlace);
            this.NotifyOfPropertyChange(() => this.CustomDetelecine);
            this.NotifyOfPropertyChange(() => this.CustomDenoise);

            this.NotifyOfPropertyChange(() => this.IsDeinterlaceMode);
            this.NotifyOfPropertyChange(() => this.IsDecombMode);
            this.NotifyOfPropertyChange(() => this.IsDeinterlaceDecomb);

            this.NotifyOfPropertyChange(() => this.FlipVideo);
            this.NotifyOfPropertyChange(() => this.SelectedRotation);

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

        #endregion
    }
}