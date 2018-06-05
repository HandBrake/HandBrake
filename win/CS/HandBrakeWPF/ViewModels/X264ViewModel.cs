// --------------------------------------------------------------------------------------------------------------------
// <copyright file="X264ViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The X264 Advanced View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;

    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Advanced View Model
    /// </summary>
    public class X264ViewModel : ViewModelBase, IX264ViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The adaptive b frames.
        /// </summary>
        private AdvancedChoice adaptiveBFrames;

        /// <summary>
        /// The adaptive quantization strength.
        /// </summary>
        private double adaptiveQuantizationStrength;

        /// <summary>
        /// The analysis.
        /// </summary>
        private AdvancedChoice analysis;

        /// <summary>
        /// The b frames.
        /// </summary>
        private AdvancedChoice bFrames;

        /// <summary>
        /// The cabac entropy coding.
        /// </summary>
        private bool cabacEntropyCoding;

        /// <summary>
        /// The deblocking strength.
        /// </summary>
        private AdvancedChoice deblockingStrength;

        /// <summary>
        /// The deblocking threshold.
        /// </summary>
        private AdvancedChoice deblockingThreshold;

        /// <summary>
        /// The direct prediction.
        /// </summary>
        private AdvancedChoice directPrediction;

        /// <summary>
        /// The eight by eight dct.
        /// </summary>
        private bool eightByEightDct;

        /// <summary>
        /// The motion estimation method.
        /// </summary>
        private AdvancedChoice motionEstimationMethod;

        /// <summary>
        /// The motion estimation range.
        /// </summary>
        private int motionEstimationRange;

        /// <summary>
        /// The no dct decimate.
        /// </summary>
        private bool noDctDecimate;

        /// <summary>
        /// The psychovisual rate distortion.
        /// </summary>
        private double psychovisualRateDistortion;

        /// <summary>
        /// The psychovisual trellis.
        /// </summary>
        private double psychovisualTrellis;

        /// <summary>
        /// The pyramidal b frames.
        /// </summary>
        private AdvancedChoice pyramidalBFrames;

        /// <summary>
        /// The reference frames.
        /// </summary>
        private AdvancedChoice referenceFrames;

        /// <summary>
        /// The subpixel motion estimation.
        /// </summary>
        private AdvancedChoice subpixelMotionEstimation;

        /// <summary>
        /// The trellis.
        /// </summary>
        private AdvancedChoice trellis;

        /// <summary>
        /// X264 options that have UI elements that correspond to them.
        /// </summary>
        private HashSet<string> uiOptions = new HashSet<string>
            {
                "ref",
                "bframes",
                "b-adapt",
                "direct",
                "weightp",
                "b-pyramid",
                "me",
                "subme",
                "subq",
                "merange",
                "analyse",
                "8x8dct",
                "cabac",
                "trellis",
                "aq-strength",
                "psy-rd",
                "no-dct-decimate",
                "deblock"
            };

        /// <summary>
        /// The weighted p frames.
        /// </summary>
        private bool weightedPFrames;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="X264ViewModel"/> class.
        /// </summary>
        public X264ViewModel()
        {
            this.Task = new EncodeTask();
            this.UpdateUIFromAdvancedOptions();
        }

        /// <summary>
        /// The task object property changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The PropertyChangedEventArgs.
        /// </param>
        private void Task_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == UserSettingConstants.ShowAdvancedTab)
            {
                ShowX264AdvancedOptions = this.Task.ShowAdvancedTab;
                this.NotifyOfPropertyChange(() => ShowX264AdvancedOptions);
                this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);

                if (ShowX264AdvancedOptions)
                {
                    this.UpdateUIFromAdvancedOptions();
                }
            }
        }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether show x 264 advanced options.
        /// </summary>
        public bool ShowX264AdvancedOptions { get; set; }

        /// <summary>
        /// Gets or sets AdaptiveBFrames.
        /// </summary>
        public AdvancedChoice AdaptiveBFrames
        {
            get
            {
                return this.adaptiveBFrames;
            }

            set
            {
                this.adaptiveBFrames = value;
                this.NotifyOfPropertyChange(() => this.AdaptiveBFrames);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets AdaptiveQuantizationStrength.
        /// </summary>
        public double AdaptiveQuantizationStrength
        {
            get
            {
                return this.adaptiveQuantizationStrength;
            }

            set
            {
                this.adaptiveQuantizationStrength = value;
                this.NotifyOfPropertyChange(() => this.AdaptiveQuantizationStrength);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets AdvancedOptionsString.
        /// </summary>
        public string AdvancedOptionsString
        {
            get
            {
                return this.Task.AdvancedEncoderOptions;
            }

            set
            {
                this.Task.AdvancedEncoderOptions = value;
                this.UpdateUIFromAdvancedOptions();
                this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);
            }
        }

        /// <summary>
        /// Gets or sets Analysis.
        /// </summary>
        public AdvancedChoice Analysis
        {
            get
            {
                return this.analysis;
            }

            set
            {
                this.analysis = value;
                this.NotifyOfPropertyChange(() => this.Analysis);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AutomaticChange.
        /// </summary>
        public bool AutomaticChange { get; set; }

        /// <summary>
        /// Gets or sets BFrames.
        /// </summary>
        public AdvancedChoice BFrames
        {
            get
            {
                return this.bFrames;
            }

            set
            {
                this.bFrames = value;
                this.NotifyOfPropertyChange(() => this.BFrames);
                this.NotifyOfPropertyChange(() => this.BFramesOptionsVisible);
                this.NotifyOfPropertyChange(() => this.PyramidalBFramesVisible);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets a value indicating whether BFramesOptionsVisible.
        /// </summary>
        public bool BFramesOptionsVisible
        {
            get
            {
                return this.BFrames.Value != "0";
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether CabacEntropyCoding.
        /// </summary>
        public bool CabacEntropyCoding
        {
            get
            {
                return this.cabacEntropyCoding;
            }

            set
            {
                this.cabacEntropyCoding = value;
                this.NotifyOfPropertyChange(() => this.CabacEntropyCoding);
                this.NotifyOfPropertyChange(() => this.PsychovisualTrellisVisible);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets DeblockingStrength.
        /// </summary>
        public AdvancedChoice DeblockingStrength
        {
            get
            {
                return this.deblockingStrength;
            }

            set
            {
                this.deblockingStrength = value;
                this.NotifyOfPropertyChange(() => this.DeblockingStrength);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets DeblockingThreshold.
        /// </summary>
        public AdvancedChoice DeblockingThreshold
        {
            get
            {
                return this.deblockingThreshold;
            }

            set
            {
                this.deblockingThreshold = value;
                this.NotifyOfPropertyChange(() => this.DeblockingThreshold);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets DirectPrediction.
        /// </summary>
        public AdvancedChoice DirectPrediction
        {
            get
            {
                return this.directPrediction;
            }

            set
            {
                this.directPrediction = value;
                this.NotifyOfPropertyChange(() => this.DirectPrediction);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether EightByEightDct.
        /// </summary>
        public bool EightByEightDct
        {
            get
            {
                return this.eightByEightDct;
            }

            set
            {
                this.eightByEightDct = value;
                this.NotifyOfPropertyChange(() => this.EightByEightDct);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets MotionEstimationMethod.
        /// </summary>
        public AdvancedChoice MotionEstimationMethod
        {
            get
            {
                return this.motionEstimationMethod;
            }

            set
            {
                this.motionEstimationMethod = value;
                this.NotifyOfPropertyChange(() => this.MotionEstimationMethod);

                if ((MotionEstimationMethod.Value == "hex" || MotionEstimationMethod.Value == "dia") && (motionEstimationRange > 16))
                {
                    this.motionEstimationRange = 16;
                    this.NotifyOfPropertyChange(() => this.MotionEstimationRange);
                }

                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets MotionEstimationRange.
        /// </summary>
        public int MotionEstimationRange
        {
            get
            {
                return this.motionEstimationRange;
            }

            set
            {
                if ((MotionEstimationMethod.Value == "hex" || MotionEstimationMethod.Value == "dia") && (value > 16))
                {
                    this.motionEstimationRange = 16;
                }
                else if (value < 4)
                {
                    this.motionEstimationRange = 4;
                }
                else
                {
                    this.motionEstimationRange = value;
                }

                this.NotifyOfPropertyChange(() => this.MotionEstimationRange);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether NoDctDecimate.
        /// </summary>
        public bool NoDctDecimate
        {
            get
            {
                return this.noDctDecimate;
            }

            set
            {
                this.noDctDecimate = value;
                this.NotifyOfPropertyChange(() => this.NoDctDecimate);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets PsychovisualRateDistortion.
        /// </summary>
        public double PsychovisualRateDistortion
        {
            get
            {
                return this.psychovisualRateDistortion;
            }

            set
            {
                this.psychovisualRateDistortion = value;
                this.NotifyOfPropertyChange(() => this.PsychovisualRateDistortion);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets PsychovisualTrellis.
        /// </summary>
        public double PsychovisualTrellis
        {
            get
            {
                return this.psychovisualTrellis;
            }

            set
            {
                this.psychovisualTrellis = value;
                this.NotifyOfPropertyChange(() => this.PsychovisualTrellis);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets a value indicating whether PsychovisualTrellisVisible.
        /// </summary>
        public bool PsychovisualTrellisVisible
        {
            get
            {
                return this.CabacEntropyCoding && this.Trellis.Value != "0";
            }
        }

        /// <summary>
        /// Gets a value indicating whether PsychovisualRateDistortionVisible.
        /// </summary>
        public bool PsychovisualRateDistortionVisible
        {
            get
            {
                int value;
                int.TryParse(this.SubpixelMotionEstimation.Value.Trim(), out value);
                return value >= 6;
            }
        }

        /// <summary>
        /// Gets or sets PyramidalBFrames.
        /// </summary>
        public AdvancedChoice PyramidalBFrames
        {
            get
            {
                return this.pyramidalBFrames;
            }

            set
            {
                this.pyramidalBFrames = value;
                this.NotifyOfPropertyChange(() => this.PyramidalBFrames);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets a value indicating whether PyramidalBFramesVisible.
        /// </summary>
        public bool PyramidalBFramesVisible
        {
            get
            {
                return int.Parse(this.BFrames.Value) > 1;
            }
        }

        /// <summary>
        /// Gets or sets ReferenceFrames.
        /// </summary>
        public AdvancedChoice ReferenceFrames
        {
            get
            {
                return this.referenceFrames;
            }

            set
            {
                this.referenceFrames = value;
                this.NotifyOfPropertyChange(() => this.ReferenceFrames);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets SubpixelMotionEstimation.
        /// </summary>
        public AdvancedChoice SubpixelMotionEstimation
        {
            get
            {
                return this.subpixelMotionEstimation;
            }

            set
            {
                this.subpixelMotionEstimation = value;
                this.NotifyOfPropertyChange(() => this.SubpixelMotionEstimation);
                this.NotifyOfPropertyChange(() => this.PsychovisualRateDistortionVisible);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets Trellis.
        /// </summary>
        public AdvancedChoice Trellis
        {
            get
            {
                return this.trellis;
            }

            set
            {
                this.trellis = value;
                this.NotifyOfPropertyChange(() => this.Trellis);
                this.NotifyOfPropertyChange(() => this.PsychovisualTrellisVisible);
                this.UpdateOptionsString();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether WeightedPFrames.
        /// </summary>
        public bool WeightedPFrames
        {
            get
            {
                return this.weightedPFrames;
            }

            set
            {
                this.weightedPFrames = value;
                this.NotifyOfPropertyChange(() => this.WeightedPFrames);
                this.UpdateOptionsString();
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// The update ui from advanced options.
        /// </summary>
        public void UpdateUIFromAdvancedOptions()
        {
            this.AutomaticChange = true;

            // Reset UI to defaults, and re-apply options.
            this.SetAdvancedToDefaults();

            if (this.Task.AdvancedEncoderOptions == null)
            {
                this.AutomaticChange = false;
                return;
            }

            // Check the updated options string. Update UI for any recognized options.
            string[] newOptionsSegments = this.Task.AdvancedEncoderOptions.Split(':');
            foreach (string newOptionsSegment in newOptionsSegments)
            {
                int equalsIndex = newOptionsSegment.IndexOf('=');
                if (equalsIndex >= 0)
                {
                    string optionName = newOptionsSegment.Substring(0, equalsIndex);
                    string optionValue = newOptionsSegment.Substring(equalsIndex + 1);

                    if (optionName != string.Empty && optionValue != string.Empty)
                    {
                        AdvancedChoice newChoice;
                        int parseInt;
                        double parseDouble;
                        string[] subParts;

                        switch (optionName)
                        {
                            case "ref":
                                if (int.TryParse(optionValue, out parseInt))
                                {
                                    newChoice =
                                        AdvancedChoicesHelper.ReferenceFrames.SingleOrDefault(
                                            choice => choice.Value == parseInt.ToString(CultureInfo.InvariantCulture));
                                    if (newChoice != null)
                                    {
                                        this.ReferenceFrames = newChoice;
                                    }
                                }

                                break;
                            case "bframes":
                                if (int.TryParse(optionValue, out parseInt))
                                {
                                    newChoice =
                                        AdvancedChoicesHelper.BFrames.SingleOrDefault(
                                            choice => choice.Value == parseInt.ToString(CultureInfo.InvariantCulture));
                                    if (newChoice != null)
                                    {
                                        this.BFrames = newChoice;
                                    }
                                }

                                break;
                            case "b-adapt":
                                newChoice =
                                    AdvancedChoicesHelper.AdaptiveBFrames.SingleOrDefault(
                                        choice => choice.Value == optionValue);
                                if (newChoice != null)
                                {
                                    this.AdaptiveBFrames = newChoice;
                                }

                                break;
                            case "direct":
                                newChoice =
                                    AdvancedChoicesHelper.DirectPrediction.SingleOrDefault(
                                        choice => choice.Value == optionValue);
                                if (newChoice != null)
                                {
                                    this.DirectPrediction = newChoice;
                                }

                                break;
                            case "weightp":
                                if (optionValue == "0")
                                {
                                    this.WeightedPFrames = false;
                                }
                                else if (optionValue == "1")
                                {
                                    this.WeightedPFrames = true;
                                }

                                break;
                            case "b-pyramid":
                                newChoice =
                                    AdvancedChoicesHelper.PyramidalBFrames.SingleOrDefault(
                                        choice => choice.Value == optionValue);
                                if (newChoice != null)
                                {
                                    this.PyramidalBFrames = newChoice;
                                }

                                break;
                            case "me":
                                newChoice =
                                    AdvancedChoicesHelper.MotionEstimationMethod.SingleOrDefault(
                                        choice => choice.Value == optionValue);
                                if (newChoice != null)
                                {
                                    this.MotionEstimationMethod = newChoice;
                                }

                                break;
                            case "subme":
                            case "subq":
                                if (int.TryParse(optionValue, out parseInt))
                                {
                                    newChoice =
                                        AdvancedChoicesHelper.SubpixelMotionEstimation.SingleOrDefault(
                                            choice => choice.Value == parseInt.ToString(CultureInfo.InvariantCulture));
                                    if (newChoice != null)
                                    {
                                        this.SubpixelMotionEstimation = newChoice;
                                    }
                                }

                                break;
                            case "merange":
                                if (int.TryParse(optionValue, out parseInt))
                                {
                                    this.MotionEstimationRange = parseInt;
                                }

                                break;
                            case "analyse":
                                newChoice =
                                    AdvancedChoicesHelper.Analysis.SingleOrDefault(
                                        choice => choice.Value == optionValue);
                                if (newChoice != null)
                                {
                                    this.Analysis = newChoice;
                                }

                                break;
                            case "8x8dct":
                                if (optionValue == "0")
                                {
                                    this.EightByEightDct = false;
                                }
                                else if (optionValue == "1")
                                {
                                    this.EightByEightDct = true;
                                }

                                break;
                            case "cabac":
                                if (optionValue == "0")
                                {
                                    this.CabacEntropyCoding = false;
                                }
                                else if (optionValue == "1")
                                {
                                    this.CabacEntropyCoding = true;
                                }

                                break;
                            case "trellis":
                                if (int.TryParse(optionValue, out parseInt))
                                {
                                    newChoice =
                                        AdvancedChoicesHelper.Trellis.SingleOrDefault(
                                            choice => choice.Value == parseInt.ToString(CultureInfo.InvariantCulture));
                                    if (newChoice != null)
                                    {
                                        this.Trellis = newChoice;
                                    }
                                }

                                break;
                            case "aq-strength":
                                if (double.TryParse(optionValue, NumberStyles.Any, CultureInfo.InvariantCulture, out parseDouble) && parseDouble >= 0.0 &&
                                    parseDouble <= 2.0)
                                {
                                    this.AdaptiveQuantizationStrength = Math.Round(parseDouble, 1);
                                }

                                break;
                            case "psy-rd":
                                subParts = optionValue.Split(',');
                                if (subParts.Length == 2)
                                {
                                    double psyRD, psyTrellis;
                                    if (double.TryParse(subParts[0], NumberStyles.Any, CultureInfo.InvariantCulture, out psyRD) &&
                                        double.TryParse(subParts[1], NumberStyles.Any, CultureInfo.InvariantCulture, out psyTrellis))
                                    {
                                        if (psyRD >= 0.0 && psyRD <= 2.0 && psyTrellis >= 0.0 && psyTrellis <= 1.0)
                                        {
                                            this.PsychovisualRateDistortion = Math.Round(psyRD, 1);
                                            this.PsychovisualTrellis = Math.Round(psyTrellis, 2);
                                        }
                                    }
                                }

                                break;
                            case "no-dct-decimate":
                                if (optionValue == "0")
                                {
                                    this.NoDctDecimate = false;
                                }
                                else if (optionValue == "1")
                                {
                                    this.NoDctDecimate = true;
                                }

                                break;
                            case "deblock":
                                subParts = optionValue.Split(',');
                                if (subParts.Length == 2)
                                {
                                    int dbStrength, dbThreshold;
                                    if (int.TryParse(subParts[0], out dbStrength) &&
                                        int.TryParse(subParts[1], out dbThreshold))
                                    {
                                        newChoice =
                                            AdvancedChoicesHelper.DeblockingStrength.SingleOrDefault(
                                                choice => choice.Value == subParts[0]);
                                        if (newChoice != null)
                                        {
                                            this.DeblockingStrength = newChoice;
                                        }

                                        newChoice =
                                            AdvancedChoicesHelper.DeblockingThreshold.SingleOrDefault(
                                                choice => choice.Value == subParts[1]);
                                        if (newChoice != null)
                                        {
                                            this.DeblockingThreshold = newChoice;
                                        }
                                    }
                                }

                                break;
                        }
                    }
                }
            }

            this.AutomaticChange = false;
        }

        #endregion

        #region Implemented Interfaces

        #region IAdvancedViewModel

        /// <summary>
        /// The set encoder.
        /// </summary>
        /// <param name="encoder">
        /// The encoder.
        /// </param>
        public void SetEncoder(VideoEncoder encoder)
        {   
        }

        /// <summary>
        /// The clear.
        /// </summary>
        public void Clear()
        {
            this.AdvancedOptionsString = string.Empty;
        }

        #endregion

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
            this.Task.PropertyChanged -= this.Task_PropertyChanged;
            this.Task = task;
            this.Task.PropertyChanged += this.Task_PropertyChanged;
            this.AdvancedOptionsString = preset.Task.AdvancedEncoderOptions;

            if (task.ShowAdvancedTab && (task.VideoEncoder == VideoEncoder.X264 || task.VideoEncoder == VideoEncoder.X264_10))
            {
                this.ShowX264AdvancedOptions = true;
                this.NotifyOfPropertyChange(() => ShowX264AdvancedOptions);
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
            this.Task = task;
            this.SetEncoder(task.VideoEncoder);
            this.AdvancedOptionsString = task.AdvancedEncoderOptions;
        }

        public bool MatchesPreset(Preset preset)
        {
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
            this.Task = task;
            this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);
        }

        #endregion

        #endregion

        #region Methods

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        /// <summary>
        /// The set advanced to defaults.
        /// </summary>
        private void SetAdvancedToDefaults()
        {
            this.ReferenceFrames = AdvancedChoicesHelper.ReferenceFrames.SingleOrDefault(choice => choice.IsDefault);
            this.BFrames = AdvancedChoicesHelper.BFrames.SingleOrDefault(choice => choice.IsDefault);
            this.AdaptiveBFrames = AdvancedChoicesHelper.AdaptiveBFrames.SingleOrDefault(choice => choice.IsDefault);
            this.DirectPrediction = AdvancedChoicesHelper.DirectPrediction.SingleOrDefault(choice => choice.IsDefault);
            this.WeightedPFrames = true;
            this.PyramidalBFrames = AdvancedChoicesHelper.PyramidalBFrames.SingleOrDefault(choice => choice.IsDefault);
            this.MotionEstimationMethod =
                AdvancedChoicesHelper.MotionEstimationMethod.SingleOrDefault(choice => choice.IsDefault);
            this.SubpixelMotionEstimation =
                AdvancedChoicesHelper.SubpixelMotionEstimation.SingleOrDefault(choice => choice.IsDefault);
            this.MotionEstimationRange = 16;
            this.Analysis = AdvancedChoicesHelper.Analysis.SingleOrDefault(choice => choice.IsDefault);
            this.EightByEightDct = true;
            this.CabacEntropyCoding = true;
            this.Trellis = AdvancedChoicesHelper.Trellis.SingleOrDefault(choice => choice.IsDefault);
            this.AdaptiveQuantizationStrength = 1.0;
            this.PsychovisualRateDistortion = 1.0;
            this.PsychovisualTrellis = 0.0;
            this.DeblockingStrength =
                AdvancedChoicesHelper.DeblockingStrength.SingleOrDefault(choice => choice.IsDefault);
            this.DeblockingThreshold =
                AdvancedChoicesHelper.DeblockingThreshold.SingleOrDefault(choice => choice.IsDefault);
            this.NoDctDecimate = false;
        }

        /// <summary>
        /// Update the x264 options string from a UI change.
        /// </summary>
        private void UpdateOptionsString()
        {
            if (this.AutomaticChange)
            {
                return;
            }

            List<string> newOptions = new List<string>();

            // First add any parts of the options string that don't correspond to the UI
            if (this.AdvancedOptionsString != null)
            {
                string[] existingSegments = this.AdvancedOptionsString.Split(':');
                foreach (string existingSegment in existingSegments)
                {
                    string optionName = existingSegment;
                    int equalsIndex = existingSegment.IndexOf('=');
                    if (equalsIndex >= 0)
                    {
                        optionName = existingSegment.Substring(0, existingSegment.IndexOf("=", System.StringComparison.Ordinal));
                    }

                    if (!this.uiOptions.Contains(optionName) && optionName != string.Empty)
                    {
                        newOptions.Add(existingSegment);
                    }
                }
            }

            // Now add everything from the UI
            if (!this.ReferenceFrames.IsDefault)
            {
                newOptions.Add("ref=" + this.ReferenceFrames.Value);
            }

            if (!this.BFrames.IsDefault)
            {
                newOptions.Add("bframes=" + this.BFrames.Value);
            }

            if (this.BFrames.Value != "0")
            {
                if (!this.AdaptiveBFrames.IsDefault)
                {
                    newOptions.Add("b-adapt=" + this.AdaptiveBFrames.Value);
                }

                if (!this.DirectPrediction.IsDefault)
                {
                    newOptions.Add("direct=" + this.DirectPrediction.Value);
                }

                if (this.BFrames.Value != "1" && !this.PyramidalBFrames.IsDefault)
                {
                    newOptions.Add("b-pyramid=" + this.PyramidalBFrames.Value);
                }
            }

            if (!this.WeightedPFrames)
            {
                newOptions.Add("weightp=0");
            }

            if (!this.MotionEstimationMethod.IsDefault)
            {
                newOptions.Add("me=" + this.MotionEstimationMethod.Value);
            }

            if (!this.SubpixelMotionEstimation.IsDefault)
            {
                newOptions.Add("subme=" + this.SubpixelMotionEstimation.Value);
            }

            if (this.MotionEstimationRange != 16)
            {
                newOptions.Add("merange=" + this.MotionEstimationRange);
            }

            if (!this.Analysis.IsDefault)
            {
                newOptions.Add("analyse=" + this.Analysis.Value);
            }

            if (this.Analysis.Value != "none" && !this.EightByEightDct)
            {
                newOptions.Add("8x8dct=0");
            }

            if (!this.CabacEntropyCoding)
            {
                newOptions.Add("cabac=0");
            }

            if (!this.Trellis.IsDefault)
            {
                newOptions.Add("trellis=" + this.Trellis.Value);
            }

            double psTrellis = 0.0;
            if (this.CabacEntropyCoding && this.Trellis.Value != "0")
            {
                psTrellis = this.PsychovisualTrellis;
            }

            if (this.AdaptiveQuantizationStrength != 1.0)
            {
                newOptions.Add(
                    "aq-strength=" + this.AdaptiveQuantizationStrength.ToString("F1", CultureInfo.InvariantCulture));
            }

            if (this.PsychovisualRateDistortion != 1.0 || psTrellis > 0.0)
            {
                newOptions.Add(
                    "psy-rd=" + this.PsychovisualRateDistortion.ToString("F1", CultureInfo.InvariantCulture) + "," +
                    psTrellis.ToString("F2", CultureInfo.InvariantCulture));
            }

            if (this.NoDctDecimate)
            {
                newOptions.Add("no-dct-decimate=1");
            }

            if (!this.DeblockingStrength.IsDefault || !this.DeblockingThreshold.IsDefault)
            {
                newOptions.Add("deblock=" + this.DeblockingStrength.Value + "," + this.DeblockingThreshold.Value);
            }

            this.Task.AdvancedEncoderOptions = string.Join(":", newOptions);
            this.NotifyOfPropertyChange(() => this.AdvancedOptionsString);
        }

        #endregion
    }
}