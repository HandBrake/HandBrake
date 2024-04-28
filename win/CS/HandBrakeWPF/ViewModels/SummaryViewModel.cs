// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SummaryViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Summary View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Windows.Media.Imaging;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModelItems.Filters;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class SummaryViewModel : ViewModelBase, ISummaryViewModel
    {
        private readonly IScan scanService;
        private readonly IUserSettingService userSettingService;

        private Preset preset;
        private EncodeTask task;
        private Source source;
        private Title currentTitle;
        private int selectedPreview = 2;

        private bool isPreviousPreviewControlVisible;
        private bool isNextPreviewControlVisible;

        private bool showPreview;

        private DelayedActionProcessor previewDelayProcessor = new DelayedActionProcessor();

        public SummaryViewModel(IScan scanService, IUserSettingService userSettingService)
        {
            this.scanService = scanService;
            this.userSettingService = userSettingService;
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;
        public event EventHandler<OutputFormatChangedEventArgs> OutputFormatChanged;

        public Preset Preset
        {
            get
            {
                return this.preset;
            }

            private set
            {
                if (Equals(value, this.preset)) return;
                this.preset = value;
                this.NotifyOfPropertyChange(() => this.Preset);
            }
        }

        public EncodeTask Task
        {
            get
            {
                return this.task;
            }

            set
            {
                if (Equals(value, this.task)) return;
                this.task = value;
                this.NotifyOfPropertyChange(() => this.Task);
            }
        }

        public Source Source
        {
            get
            {
                return this.source;
            }

            set
            {
                if (Equals(value, this.source)) return;
                this.source = value;
                this.NotifyOfPropertyChange(() => this.Source);
            }
        }

        public Title CurrentTitle
        {
            get
            {
                return this.currentTitle;
            }

            set
            {
                if (Equals(value, this.currentTitle)) return;
                this.currentTitle = value;
                this.NotifyOfPropertyChange(() => this.CurrentTitle);
            }
        }

        public IEnumerable<OutputFormat> OutputFormats
        {
            get
            {
                return new List<OutputFormat>
                       {
                           OutputFormat.Mp4, OutputFormat.Mkv, OutputFormat.WebM
                       };
            }
        }

        #region DisplayProperties

        public BitmapSource PreviewImage { get; set; }

        public bool PreviewNotAvailable { get; set; }

        public int MaxWidth { get; set; }

        public int MaxHeight { get; set; }

        public string VideoTrackInfo { get; set; }

        public string AudioTrackInfo { get; set; }

        public string SubtitleTrackInfo { get; set; }

        public string ChapterInfo { get; set; }

        public string FiltersInfo { get; set; }

        public string DimensionInfo { get; set; }

        public string AspectInfo { get; set; }

        public bool IsPreviewInfoVisible { get; set; }

        public string PreviewInfo { get; set; }

        public bool IsPreviousPreviewControlVisible
        {
            get
            {
                return this.isPreviousPreviewControlVisible;
            }

            set
            {
                if (value == this.isPreviousPreviewControlVisible) return;
                this.isPreviousPreviewControlVisible = value;
                this.NotifyOfPropertyChange(() => this.IsPreviousPreviewControlVisible);
            }
        }

        public bool IsNextPreviewControlVisible
        {
            get
            {
                return this.isNextPreviewControlVisible;
            }

            set
            {
                if (value == this.isNextPreviewControlVisible) return;
                this.isNextPreviewControlVisible = value;
                this.NotifyOfPropertyChange(() => this.IsNextPreviewControlVisible);
            }
        }

        public bool ShowPreview
        {
            get
            {
                return this.showPreview;
            }

            set
            {
                if (value == this.showPreview) return;
                this.showPreview = value;
                this.NotifyOfPropertyChange(() => this.ShowPreview);
            }
        }

        #endregion

        #region Task Properties 

        /// <summary>
        /// Gets or sets SelectedOutputFormat.
        /// </summary>
        public OutputFormat SelectedOutputFormat
        {
            get
            {
                return this.Task?.OutputFormat ?? OutputFormat.Mp4; 
            }

            set
            {
                if (this.Task != null && !Equals(this.Task.OutputFormat, value))
                {
                    this.Task.OutputFormat = value;
                    this.Task.OutputFormat = value;
                    this.NotifyOfPropertyChange(() => this.SelectedOutputFormat);
                    this.NotifyOfPropertyChange(() => this.Task.OutputFormat);
                    this.NotifyOfPropertyChange(() => this.IsMkvOrWebm);
                    this.NotifyOfPropertyChange(() => this.IsIpodAtomVisible);
                    this.SetExtension(string.Format(".{0}", this.Task.OutputFormat.ToString().ToLower()));
                    this.UpdateDisplayedInfo(); // output format may be coerced to another due to container incompatibility

                    this.OnOutputFormatChanged(new OutputFormatChangedEventArgs(null));
                    this.OnTabStatusChanged(null);
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether IsMkvOrWebm.
        /// </summary>
        public bool IsMkvOrWebm
        {
            get
            {
                return this.SelectedOutputFormat == OutputFormat.Mkv || this.SelectedOutputFormat == OutputFormat.WebM;
            }
        }

        public bool IsIpodAtomVisible
        {
            get
            {
                if (this.task == null)
                {
                    return false;
                }

                return this.SelectedOutputFormat == OutputFormat.Mp4 && this.task.VideoEncoder != null && this.task.VideoEncoder.IsH264;
            }
        }

        /// <summary>
        /// Optimise Checkbox
        /// </summary>
        public bool Optimize
        {
            get
            {
                return this.Task?.Optimize ?? false;
            }
            set
            {
                if (value == this.Task.Optimize)
                {
                    return;
                }
                this.Task.Optimize = value;
                this.NotifyOfPropertyChange(() => this.Optimize);
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// iPod 5G Status
        /// </summary>
        public bool IPod5GSupport
        {
            get
            {
                return this.Task?.IPod5GSupport ?? false;
            }

            set
            {
                if (value == this.Task.IPod5GSupport)
                {
                    return;
                }
                this.Task.IPod5GSupport = value;
                this.NotifyOfPropertyChange(() => this.IPod5GSupport);
                this.OnTabStatusChanged(null);
            }
        }

        public bool AlignAVStart
        {
            get
            {
                return this.Task?.AlignAVStart ?? false;
            }
            set
            {
                if (value == this.Task.AlignAVStart)
                {
                    return;
                }
                this.Task.AlignAVStart = value;
                this.NotifyOfPropertyChange(() => this.AlignAVStart);
                this.OnTabStatusChanged(null);
            }
        }

        public bool MetadataPassthru
        {
            get => this.task?.PassthruMetadataEnabled ?? false;
            set
            {
                this.task.PassthruMetadataEnabled = value;
                this.NotifyOfPropertyChange(() => this.MetadataPassthru);
            }
        }

        #endregion

        public void SetSource(Source scannedSource, Title selectedTitle, Preset currentPreset, EncodeTask encodeTask)
        {
            this.Source = scannedSource;
            this.CurrentTitle = selectedTitle;
            this.Task = encodeTask;
            this.UpdateDisplayedInfo();
            this.SetPreviewControlVisibility();
        }

        public void SetPreset(Preset currentPreset, EncodeTask encodeTask)
        {
            this.Preset = currentPreset;
            this.Task = encodeTask;
            this.UpdateSettings(currentPreset);
            this.UpdateDisplayedInfo();
        }

        public void UpdateTask(EncodeTask updatedTask)
        {
            this.Task = updatedTask;
            this.UpdateDisplayedInfo();

            this.NotifyOfPropertyChange(() => this.SelectedOutputFormat);
            this.NotifyOfPropertyChange(() => this.IsMkvOrWebm);
            this.NotifyOfPropertyChange(() => this.IsIpodAtomVisible);

            this.NotifyOfPropertyChange(() => this.Optimize);
            this.NotifyOfPropertyChange(() => this.IPod5GSupport);
            this.NotifyOfPropertyChange(() => this.AlignAVStart);
            this.NotifyOfPropertyChange(() => this.MetadataPassthru);
        }

        public bool MatchesPreset(Preset preset)
        {
            if (preset.Task.OutputFormat != this.SelectedOutputFormat)
            {
                return false;
            }

            if (preset.Task.Optimize != this.Optimize)
            {
                return false;
            }

            if (preset.Task.AlignAVStart != this.AlignAVStart)
            {
                return false;
            }

            if (preset.Task.IPod5GSupport != this.IPod5GSupport)
            {
                return false;
            }

            return true;
        }

        public void UpdateDisplayedInfo()
        {
            if (this.CurrentTitle == null)
            {
                this.ClearDisplay();
                return;
            }

            this.PopulateSummaryTab();
            if (this.ShowPreview)
            {
                this.previewDelayProcessor.PerformTask(this.UpdatePreviewFrame, 250);
            }
        }

        public void SetContainer(OutputFormat container)
        {
            this.SelectedOutputFormat = container;
        }

        public void NextPreview()
        {
            int maxPreview = this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);
            if (this.selectedPreview == maxPreview)
            {
                return;
            }

            this.selectedPreview = this.selectedPreview + 1;
            this.UpdatePreviewFrame();
            this.PreviewInfo = string.Format(Resources.SummaryView_PreviewInfo, this.selectedPreview, maxPreview);
            this.NotifyOfPropertyChange(() => this.PreviewInfo);

            this.SetPreviewControlVisibility();
        }

        public void PreviousPreview()
        {
            int maxPreview = this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);
            if (this.selectedPreview <= 1)
            {
                return;
            }

            this.selectedPreview = this.selectedPreview - 1;
            this.UpdatePreviewFrame();
            this.PreviewInfo = string.Format(Resources.SummaryView_PreviewInfo, this.selectedPreview, maxPreview);
            this.NotifyOfPropertyChange(() => this.PreviewInfo);

            this.SetPreviewControlVisibility();
        }

        public void SetPreviewControlVisibility()
        {
            if (this.selectedPreview > 1)
            {
                this.IsPreviousPreviewControlVisible = true;
            }
            else
            {
                this.IsPreviousPreviewControlVisible = false;
            }

            if (this.selectedPreview < this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount))
            {
                this.IsNextPreviewControlVisible = true;
            }
            else
            {
                this.IsNextPreviewControlVisible = false;
            }
        }

        #region Private Methods

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        private void UpdateSettings(Preset selectedPreset)
        {
            // Main Window Settings
            this.SelectedOutputFormat = selectedPreset.Task.OutputFormat;
            this.Optimize = selectedPreset.Task.Optimize;
            this.IPod5GSupport = selectedPreset.Task.IPod5GSupport;
            this.AlignAVStart = selectedPreset.Task.AlignAVStart;
            this.MetadataPassthru = selectedPreset.Task?.PassthruMetadataEnabled ?? false;
        }

        private void SetExtension(string newExtension)
        {
            // Make sure the output extension is set correctly based on the users preferences and selection.
            if (newExtension == ".mp4" || newExtension == ".m4v")
            {
                switch ((Mp4Behaviour)this.userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                {
                    case Mp4Behaviour.MP4: // MP4
                        newExtension = ".mp4";
                        break;
                    case Mp4Behaviour.M4V: // M4v
                        newExtension = ".m4v";
                        break;
                }
            }

            // Now disable controls that are not required. The Following are for MP4 only!
            if (newExtension == ".mkv" || newExtension == ".webm")
            {
                this.IPod5GSupport = false;
                this.AlignAVStart = false;
            }

            if (this.task.VideoEncoder == null || !this.task.VideoEncoder.IsH264)
            {
                this.IPod5GSupport = false;
            }

            this.NotifyOfPropertyChange(() => this.IsMkvOrWebm);
            this.NotifyOfPropertyChange(() => this.IsIpodAtomVisible);

            // Update The browse file extension display
            if (Path.HasExtension(newExtension))
            {
                this.OnOutputFormatChanged(new OutputFormatChangedEventArgs(newExtension));
            }

            // Update the UI Display
            this.NotifyOfPropertyChange(() => this.Task);
        }

        private void PopulateSummaryTab()
        {
            if (this.Task == null)
            {
                this.ClearDisplay();
                return;
            }

            // Dimension Section
            this.VideoTrackInfo = this.Task.Framerate == null 
                                      ? string.Format("{0}, {1} FPS {2}", this.Task.VideoEncoder?.DisplayName, Resources.SummaryView_SameAsSource, this.Task.FramerateMode) 
                                      : string.Format("{0}, {1} FPS {2}", this.Task.VideoEncoder?.DisplayName, this.Task.Framerate, this.Task.FramerateMode);
            
            this.NotifyOfPropertyChange(() => this.VideoTrackInfo);

            this.AudioTrackInfo = this.GetAudioDescription();
            this.NotifyOfPropertyChange(() => this.AudioTrackInfo);

            this.SubtitleTrackInfo = this.GetSubtitleDescription();
            this.NotifyOfPropertyChange(() => this.SubtitleTrackInfo);

            this.ChapterInfo = this.Task.IncludeChapterMarkers ? Resources.SummaryView_Chapters : Resources.SummaryView_NoChapters;
            this.NotifyOfPropertyChange(() => this.ChapterInfo);

            this.FiltersInfo = this.GetFilterDescription();
            this.NotifyOfPropertyChange(() => this.FiltersInfo);

            // Picture Section
            string storageDesc = Resources.SummaryView_storage;
            int? width = this.task.Width;
            int? height = this.task.Height;
            if (this.task.Padding.Enabled)
            {
                storageDesc = string.Format("{0} {1}", Resources.SummaryView_storage, Resources.SummaryView_Padded);
                width = this.task.Width + this.task.Padding.W;
                height = this.task.Height + this.task.Padding.H;
            }
     
            this.DimensionInfo = string.Format("{0}x{1} {2}, {3}x{4} {5}", width, height, storageDesc, this.Task.DisplayWidth, this.Task.Height, Resources.SummaryView_display);
            this.NotifyOfPropertyChange(() => this.DimensionInfo);

            this.AspectInfo = string.Empty;
            this.NotifyOfPropertyChange(() => this.AspectInfo);

            // Preview
            this.PreviewInfo = string.Format(Resources.SummaryView_PreviewInfo, this.selectedPreview, this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount));
            this.NotifyOfPropertyChange(() => this.PreviewInfo);

            this.ShowPreview = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPreviewOnSummaryTab);

            this.NotifyOfPropertyChange(() => this.IsIpodAtomVisible);
        }

        private string GetFilterDescription()
        {
            if (this.Task == null)
            {
                return Resources.SummaryView_NoFilters;
            }

            List<string> filters = new List<string>();

            if (this.Task.Detelecine != Detelecine.Off)
            {
                filters.Add(Resources.SummaryView_Detelecine);
            }

            if (this.Task.DeinterlaceFilter != DeinterlaceFilter.Off)
            {
                filters.Add(EnumHelper<DeinterlaceFilter>.GetShortName(this.task.DeinterlaceFilter));
            }

            if (this.Task.Denoise != Denoise.Off)
            {
                filters.Add(this.Task.Denoise.ToString());
            }

            if (this.Task.Sharpen != Sharpen.Off)
            {
                filters.Add(this.Task.Sharpen.ToString());
            }

            if (this.Task.DeblockPreset != null && this.Task.DeblockPreset.Key != DeblockFilter.Off)
            {
                filters.Add(Resources.SummaryView_Deblock);
            }

            if (this.Task.Grayscale)
            {
                filters.Add(Resources.SummaryView_Grayscale);
            }

            if (this.Task.Rotation != 0 || this.task.FlipVideo)
            {
                filters.Add(Resources.SummaryView_Rotation);
            }

            return string.Join(", ", filters).TrimEnd(',').Trim();
        }

        private string GetAudioDescription()
        {
            if (this.Task.AudioTracks.Count == 0)
            {
                return Resources.SummaryView_NoAudioTracks;
            }

            StringBuilder desc = new StringBuilder();

            if (this.Task.AudioTracks.Count >= 1)
            {
                AudioTrack track1 = this.Task.AudioTracks[0];
                HBMixdown mixdownName = HandBrakeEncoderHelpers.GetMixdown(track1.MixDown);
                string mixdown = mixdownName != null ? ", " + mixdownName.DisplayName : string.Empty;
                desc.AppendLine(string.Format("{0}{1}", track1.Encoder.DisplayName, mixdown));
            }

            if (this.Task.AudioTracks.Count >= 2)
            {
                AudioTrack track2 = this.Task.AudioTracks[1];
                HBMixdown mixdownName = HandBrakeEncoderHelpers.GetMixdown(track2.MixDown);
                string mixdown = mixdownName != null ? ", " + mixdownName.DisplayName : string.Empty;
                desc.AppendLine(string.Format("{0}{1}", track2.Encoder.DisplayName, mixdown));
            }

            if (this.Task.AudioTracks.Count > 2)
            {
                desc.AppendLine(string.Format("+ {0} {1}", this.Task.AudioTracks.Count - 2, Resources.SummaryView_AdditionalAudioTracks));
            }

            return desc.ToString().Trim();        
        }
        
        private string GetSubtitleDescription()
        {
            if (this.Task.AudioTracks.Count == 0)
            {
                return Resources.SummaryView_NoSubtitleTracks;
            }

            StringBuilder desc = new StringBuilder();

            if (this.Task.SubtitleTracks.Count >= 1)
            {
                SubtitleTrack track1 = this.Task.SubtitleTracks[0];
                string subtitleName = track1.IsSrtSubtitle ? track1.SrtFileName : track1.SourceTrack.ToString();
                string burned = track1.Burned ? ", " + Resources.SummaryView_Burned : string.Empty;
                desc.AppendLine(string.Format("{0}{1}", subtitleName, burned));
            }

            if (this.Task.SubtitleTracks.Count >= 2)
            {
                SubtitleTrack track2 = this.Task.SubtitleTracks[1];
                string subtitleName = track2.IsSrtSubtitle ? track2.SrtFileName : track2.SourceTrack.ToString();
                string burned = track2.Burned ? ", " + Resources.SummaryView_Burned : string.Empty;
                desc.AppendLine(string.Format("{0}{1}", subtitleName, burned));
            }

            if (this.Task.SubtitleTracks.Count > 2)
            {
                desc.AppendLine(string.Format("+ {0} {1}", this.Task.SubtitleTracks.Count - 2, Resources.SummaryView_AdditionalSubtitleTracks));
            }

            return desc.ToString().Trim();
        }

        private void ClearDisplay()
        {
            this.VideoTrackInfo = Resources.SummaryView_NoTracks;
            this.NotifyOfPropertyChange(() => this.VideoTrackInfo);

            this.AudioTrackInfo = string.Empty;
            this.NotifyOfPropertyChange(() => this.AudioTrackInfo);

            this.SubtitleTrackInfo = string.Empty;
            this.NotifyOfPropertyChange(() => this.SubtitleTrackInfo);

            this.ChapterInfo = string.Empty;
            this.NotifyOfPropertyChange(() => this.ChapterInfo);

            this.FiltersInfo = Resources.SummaryView_NoFilters;
            this.NotifyOfPropertyChange(() => this.FiltersInfo);

            this.DimensionInfo = Resources.SummaryView_NoSource;
            this.NotifyOfPropertyChange(() => this.ChapterInfo);

            this.AspectInfo = string.Empty;
            this.NotifyOfPropertyChange(() => this.FiltersInfo);
        }

        private void UpdatePreviewFrame()
        {
            // Don't preview for small images.
            if (this.Task.Width < 32)
            {
                this.PreviewNotAvailable = true;
                this.IsPreviewInfoVisible = false;
                this.NotifyOfPropertyChange(() => this.IsPreviewInfoVisible);
                return;
            }

            if ((this.Task.Anamorphic == Anamorphic.None || this.Task.Anamorphic == Anamorphic.Custom) && (this.Task.Width < 32 || this.Task.Height < 32))
            {
                this.PreviewNotAvailable = true;
                return;
            }

            BitmapSource image = null;
            try
            {
                image = this.scanService.GetPreview(this.Task, this.selectedPreview - 1); 
            }
            catch (Exception exc)
            {
                this.PreviewNotAvailable = true;
                Debug.WriteLine(exc);
            }

            if (image != null)
            {
                this.PreviewNotAvailable = false;
                this.PreviewImage = image;
                this.MaxWidth = (int)image.Width;
                this.MaxHeight = (int)image.Height;
                this.IsPreviewInfoVisible = true;
                this.NotifyOfPropertyChange(() => this.IsPreviewInfoVisible);
                this.NotifyOfPropertyChange(() => this.PreviewImage);
                this.NotifyOfPropertyChange(() => this.MaxWidth);
                this.NotifyOfPropertyChange(() => this.MaxHeight);
            }
        }

        protected virtual void OnOutputFormatChanged(OutputFormatChangedEventArgs e)
        {
            this.OutputFormatChanged?.Invoke(this, e);
        }

        #endregion
    }
}
