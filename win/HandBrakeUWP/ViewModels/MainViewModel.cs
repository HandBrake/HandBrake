// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeUWP.ViewModels
{
    using HandBrake.Services.Interfaces;
    using HandBrake.Services.Presets.Interfaces;
    using HandBrake.Services.Scan.Interfaces;
    using HandBrake.ViewModels;
    using HandBrake.ViewModels.Interfaces;

    public class MainViewModel : MainViewModelBase
    {
        public MainViewModel(IUserSettingService userSettingService, IScan scanService, IPresetService presetService, IErrorService errorService,
            IPrePostActionService whenDoneService, IPictureSettingsViewModel pictureSettingsViewModel, IVideoViewModel videoViewModel,
            ISummaryViewModel summaryViewModel, IFiltersViewModel filtersViewModel, IAudioViewModel audioViewModel,
            ISubtitlesViewModel subtitlesViewModel, IX264ViewModel advancedViewModel, IChaptersViewModel chaptersViewModel,
            IStaticPreviewViewModel staticPreviewViewModel, IQueueViewModel queueViewModel, IMetaDataViewModel metaDataViewModel)
            : base(userSettingService, scanService, presetService, errorService, whenDoneService, pictureSettingsViewModel, videoViewModel, summaryViewModel, filtersViewModel, audioViewModel, subtitlesViewModel, advancedViewModel, chaptersViewModel, staticPreviewViewModel, queueViewModel, metaDataViewModel)
        {
        }
    }
}