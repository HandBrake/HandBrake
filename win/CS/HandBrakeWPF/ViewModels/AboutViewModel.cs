// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AboutViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The About View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The About View Model
    /// </summary>
    public class AboutViewModel : ViewModelBase, IAboutViewModel
    {
        /// <summary>
        /// The system info.
        /// </summary>
        private string systemInfo;

        /// <summary>
        /// Initializes a new instance of the <see cref="AboutViewModel"/> class.
        /// </summary>
        public AboutViewModel()
        {
            this.Title = "About HandBrake";

            StringBuilder builder = new StringBuilder();
            foreach (var item in SystemInfo.GetGPUInfo)
            {
                builder.AppendLine(item);
            }

            StringBuilder system = new StringBuilder();
            system.AppendLine(string.Format("Enviroment: {0}", Environment.NewLine));
            system.AppendLine(string.Format("Operating System: {0}", Environment.OSVersion));
            system.AppendLine(string.Format("CPU: {0}", SystemInfo.GetCpuCount));
            system.AppendLine(string.Format("Ram: {0} MB{1}", SystemInfo.TotalPhysicalMemory, Environment.NewLine));

            system.AppendLine(string.Format("{0}GPU Information:{0}{0}{1}", Environment.NewLine, builder));

            system.AppendLine(string.Format("{0}System Paths:{0}", Environment.NewLine));
            system.AppendLine(string.Format("Temp Dir: {0}", Path.GetTempPath()));
            system.AppendLine(string.Format("Install Dir: {0}", Application.StartupPath));
            system.AppendLine(string.Format("Data Dir: {0}\n", Application.UserAppDataPath));

            SystemInformation = system.ToString();
        }

        /// <summary>
        /// Gets Version.
        /// </summary>
        public string Version
        {
            get
            {
                return string.Format("{0} - {1}", VersionHelper.GetVersion(), VersionHelper.GetPlatformBitnessVersion());
            }
        }

        /// <summary>
        /// Gets or sets the system info.
        /// </summary>
        public string SystemInformation
        {
            get
            {
                return this.systemInfo;
            }
            set
            {
                this.systemInfo = value;
                this.NotifyOfPropertyChange("SystemInfo");
            }
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }
    }
}
