// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UpdateVersionService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the UpdateVersionService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Security.Cryptography;
    using System.Text.RegularExpressions;
    using System.Windows;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// Update Version Service - Handlers Update and Versioning.
    /// </summary>
    public class UpdateVersionService : IUpdateVersionService
    {
        /// <summary>
        /// The Backing field for the user setting service
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Error Service Backing field.
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// Initializes a new instance of the <see cref="UpdateVersionService"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user setting service.
        /// </param>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        public UpdateVersionService(IUserSettingService userSettingService, IErrorService errorService)
        {
            this.userSettingService = userSettingService;
            this.errorService = errorService;
        }

        /// <summary>
        /// Get's HandBrakes version data from the CLI.
        /// </summary>
        public void SetCliVersionData()
        {
            string line;

            // 0 = SVN Build / Version
            // 1 = Build Date

            // Get the SHA1 Hash of HandBrakeCLI
            byte[] hash;
            string starupPath = Path.GetDirectoryName(Process.GetCurrentProcess().MainModule.FileName);

            using (Stream stream = File.OpenRead(Path.Combine(starupPath, "HandBrakeCLI.exe")))
            {
                hash = SHA1.Create().ComputeHash(stream);
            }
            string base64Hash = Convert.ToBase64String(hash);

            // Compare the hash with the last known hash. If it's the same, return.
            if (userSettingService.GetUserSetting<string>(ASUserSettingConstants.HandBrakeExeHash) == base64Hash)
            {
                return;
            }

            // It's not the same, so start the CLI to get it's version data.
            Process cliProcess = new Process();
            ProcessStartInfo handBrakeCli = new ProcessStartInfo("HandBrakeCLI.exe", " -u -v0")
            {
                UseShellExecute = false,
                RedirectStandardError = true,
                RedirectStandardOutput = true,
                CreateNoWindow = true
            };
            cliProcess.StartInfo = handBrakeCli;

            try
            {
                cliProcess.Start();

                // Retrieve standard output and report back to parent thread until the process is complete
                TextReader stdOutput = cliProcess.StandardError;

                while (!cliProcess.HasExited)
                {
                    line = stdOutput.ReadLine() ?? string.Empty;
                    Match m = Regex.Match(line, @"HandBrake ([svnM0-9.]*) \(([0-9]*)\)");
                    Match platform = Regex.Match(line, @"- ([A-Za-z0-9\s ]*) -");

                    if (m.Success)
                    {
                        string version = m.Groups[1].Success ? m.Groups[1].Value : string.Empty;
                        string build = m.Groups[2].Success ? m.Groups[2].Value : string.Empty;

                        int buildValue;
                        int.TryParse(build, out buildValue);

                        userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeBuild, buildValue);
                        userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeVersion, version);
                    }

                    if (platform.Success)
                    {
                        userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakePlatform, platform.Value.Replace("-", string.Empty).Trim());
                    }

                    if (cliProcess.TotalProcessorTime.Seconds > 10) // Don't wait longer than 10 seconds.
                    {
                        Process cli = Process.GetProcessById(cliProcess.Id);
                        if (!cli.HasExited)
                        {
                            cli.Kill();
                        }
                    }
                }

                userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeExeHash, base64Hash);
            }
            catch (Exception e)
            {
                userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeBuild, string.Empty);
                userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakePlatform, string.Empty);
                userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeVersion, string.Empty);
                userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeExeHash, string.Empty);

                this.errorService.ShowError("Unable to Initialise HandBrake", "This error is unrecoverable. Maybe try restarting.", e);

                Application.Current.Shutdown();
            }
        }
    }
}
