// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddToQueueQualitySweep.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.DebugTools
{
    using System;
    using System.IO;
    using System.Windows;
    using System.Windows.Input;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class AddToQueueQualitySweepCommand : ICommand
    {
        private readonly IUserSettingService userSettingService;
        private readonly IErrorService errorService;

        private MainViewModel mainViewModel;
        private VideoViewModel videoViewModel;


        public AddToQueueQualitySweepCommand(IMainViewModel mvm, IVideoViewModel vvm, IUserSettingService userSettingService, IErrorService errorService)
        {
            this.userSettingService = userSettingService;
            this.errorService = errorService;

            // Don't want to pollute the API for debug features.
            this.mainViewModel = mvm as MainViewModel;
            this.videoViewModel = vvm as VideoViewModel;
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            if (this.mainViewModel == null || this.videoViewModel == null)
            {
                return;
            }

            if (!this.mainViewModel.HasSource || string.IsNullOrEmpty(this.mainViewModel.Destination))
            {
                this.errorService.ShowMessageBox(Resources.Debug_QueueSweep_NotSetup, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            double step = this.userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step);
            if (step != 1)
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.X264Step, (double)1);
                this.videoViewModel.RF = 20;
            }

            string dir = Path.GetDirectoryName(this.mainViewModel.Destination);
            int highNumber = this.videoViewModel.QualityMax > this.videoViewModel.QualityMin ? this.videoViewModel.QualityMax : this.videoViewModel.QualityMin;
            int lowNumber = this.videoViewModel.QualityMax > this.videoViewModel.QualityMin ? this.videoViewModel.QualityMin : this.videoViewModel.QualityMax;
            bool flipMode = this.videoViewModel.QualityMax > this.videoViewModel.QualityMin;

            for (int i = lowNumber; i <= highNumber; i++)
            {
                int cqNumber = i;
                if (flipMode)
                {
                    cqNumber = this.videoViewModel.QualityMax - i;
                }

                string extension = Path.GetExtension(this.mainViewModel.Destination);
                string newFileName = string.Format("{0}{1}", cqNumber, extension);

                this.mainViewModel.Destination = Path.Combine(dir, newFileName);
                this.videoViewModel.RF = i;
                this.mainViewModel.AddToQueue(false);
            }

            if (step != 1)
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.X264Step, (double)step);
            }
        }

        public event EventHandler CanExecuteChanged;
    }
}
