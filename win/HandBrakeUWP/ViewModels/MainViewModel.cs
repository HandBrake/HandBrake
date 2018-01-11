// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using HandBrake.Services.Interfaces;
    using HandBrake.Services.Presets.Interfaces;
    using HandBrake.Services.Queue.Interfaces;
    using HandBrake.Services.Scan.Interfaces;
    using HandBrake.Utilities;
    using HandBrake.Utilities.Interfaces;
    using HandBrake.ViewModels.Interfaces;

    public class MainViewModel : MainViewModelBase
    {
        public MainViewModel(IUserSettingService userSettingService, IScan scanService, IPresetService presetService, IErrorService errorService,
            IViewManager viewManager, INotificationService notificationManager, IQueueProcessor queueProcessor,
            IPrePostActionService whenDoneService, IPictureSettingsViewModel pictureSettingsViewModel, IVideoViewModel videoViewModel,
            ISummaryViewModel summaryViewModel, IFiltersViewModel filtersViewModel, IAudioViewModel audioViewModel,
            ISubtitlesViewModel subtitlesViewModel, IX264ViewModel advancedViewModel, IChaptersViewModel chaptersViewModel,
            IStaticPreviewViewModel staticPreviewViewModel, IQueueViewModel queueViewModel, IMetaDataViewModel metaDataViewModel,
            ISystemInfo systemInfo, LauncherServiceBase launcher, ITaskBarService taskbarService)
            : base(userSettingService, scanService, presetService, errorService, viewManager, notificationManager, queueProcessor, whenDoneService,
                  pictureSettingsViewModel, videoViewModel, summaryViewModel, filtersViewModel, audioViewModel, subtitlesViewModel,
                  advancedViewModel, chaptersViewModel, staticPreviewViewModel, queueViewModel, metaDataViewModel, systemInfo, launcher, taskbarService)
        {
        }
    }
}