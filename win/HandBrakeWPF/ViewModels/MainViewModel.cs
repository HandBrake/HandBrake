// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Windows.Input;
    using HandBrake.Properties;
    using HandBrakeWPF.Commands.Menu;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class MainViewModel : MainViewModelBase
    {
        private readonly IUpdateService updateService;

        public MainViewModel(IUserSettingService userSettingService, IScan scanService, IPresetService presetService, IErrorService errorService,
            IPrePostActionService whenDoneService, IPictureSettingsViewModel pictureSettingsViewModel, IVideoViewModel videoViewModel,
            ISummaryViewModel summaryViewModel, IFiltersViewModel filtersViewModel, IAudioViewModel audioViewModel,
            ISubtitlesViewModel subtitlesViewModel, IX264ViewModel advancedViewModel, IChaptersViewModel chaptersViewModel,
            IStaticPreviewViewModel staticPreviewViewModel, IQueueViewModel queueViewModel, IMetaDataViewModel metaDataViewModel,
            IUpdateService updateService)
            : base(userSettingService, scanService, presetService, errorService, whenDoneService, pictureSettingsViewModel, videoViewModel,
                  summaryViewModel, filtersViewModel, audioViewModel, subtitlesViewModel, advancedViewModel, chaptersViewModel, staticPreviewViewModel, queueViewModel, metaDataViewModel)
        {
            this.updateService = updateService;

            // Setup Commands
            this.QueueCommand = new QueueCommands(this.QueueViewModel);
        }

        public bool IsUWP { get; } = UwpDetect.IsUWP();

        /// <summary>
        /// Gets or sets the queue command.
        /// </summary>
        public ICommand QueueCommand { get; set; }

        public override void OnLoad()
        {
            base.OnLoad();

            // Perform an update check if required
            this.updateService.PerformStartupUpdateCheck(this.HandleUpdateCheckResults);
        }

        /// <summary>
        /// Handle Update Check Results
        /// </summary>
        /// <param name="information">
        /// The information.
        /// </param>
        private void HandleUpdateCheckResults(UpdateCheckInformation information)
        {
            if (information.NewVersionAvailable)
            {
                this.ProgramStatusLabel = Resources.Main_NewUpdate;
            }
        }
    }
}