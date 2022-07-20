// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ErrorViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ErrorViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Error View Model
    /// </summary>
    public class ErrorViewModel : ViewModelBase, IErrorViewModel
    {
        private readonly IErrorService errorService;

        private string details;
        private string errorMessage;
        private string solution;

        public ErrorViewModel(IErrorService errorService)
        {
            this.errorService = errorService;
            this.Title = Resources.Error;
            this.ErrorMessage = Resources.ErrorViewModel_UnknownError;
            this.Details = Resources.ErrorViewModel_NoFurtherInformation; 
        }

        public string Details
        {
            get
            {
                return string.IsNullOrEmpty(this.details) ? Resources.ErrorViewModel_NoFurtherInformation : this.details;
            }

            set
            {
                this.details = value;
                this.NotifyOfPropertyChange(() =>this.Details);
                if (this.details != Resources.ErrorViewModel_NoFurtherInformation)
                {
                    this.AppendExceptionLog(this.Details);
                }
            }
        }

        public string ErrorMessage
        {
            get
            {
                return this.errorMessage;
            }

            set
            {
                this.errorMessage = value;
                this.NotifyOfPropertyChange(() => this.ErrorMessage);
            }
        }

        public string Solution
        {
            get
            {
                return string.IsNullOrEmpty(this.solution) ? Resources.ErrorViewModel_IfTheProblemPersists : this.solution;
            }

            set
            {
                this.solution = value;
                this.NotifyOfPropertyChange(() => this.Solution);
            }
        }

        public void Close()
        {
            try
            {
                this.TryClose();
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
            }
        }

        public void Copy()
        {
            try
            {
                Clipboard.SetDataObject(this.ErrorMessage + Environment.NewLine + this.Details, true);
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Clipboard_Unavailable, Resources.Clipboard_Unavailable_Solution, exc);
            }
        }

        public void AppendExceptionLog(string exc)
        {
            try
            {
                string logDir = DirectoryUtilities.GetLogDirectory();
                string logFile = Path.Combine(logDir, string.Format("exception_log{0}.txt", GeneralUtilities.ProcessId));
                using (StreamWriter sw = File.AppendText(logFile))
                {
                    sw.WriteLine(exc + Environment.NewLine);
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e);
            }
        }
    }
}