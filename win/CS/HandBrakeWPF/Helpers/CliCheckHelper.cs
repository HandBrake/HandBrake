// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CliCheckHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the CliCheckHelper type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Security.Cryptography;
    using System.Text.RegularExpressions;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// The cli check helper.
    /// </summary>
    public class CliCheckHelper
    {
        /// <summary>
        /// The check cli version.
        /// </summary>
        public static void CheckCLIVersion()
        {
            IErrorService errorService = IoC.Get<IErrorService>();

            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();

            string line;

            // 0 = SVN Build / Version
            // 1 = Build Date

            // Get the SHA1 Hash of HandBrakeCLI
            byte[] hash;
            using (Stream stream = File.OpenRead("HandBrakeCLI.exe"))
            {
                hash = SHA1.Create().ComputeHash(stream);
            }

            string base64Hash = Convert.ToBase64String(hash);

            // Compare the hash with the last known hash. If it's the same, return.
            if (userSettingService.GetUserSetting<string>(UserSettingConstants.HandBrakeExeHash) == base64Hash)
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
                bool success = false;
                TextReader stdOutput = cliProcess.StandardError;
                while ((line = stdOutput.ReadLine()) != null)
                {
                    Match m = Regex.Match(line, @"HandBrake ([svnM0-9.]*) \(([0-9]*)\)");
                    if (m.Success)
                    {
                        string build = m.Groups[2].Success ? m.Groups[2].Value : string.Empty;

                        int buildValue;
                        int.TryParse(build, out buildValue);

                        userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeBuild, buildValue);
                        success = true;
                    }
                }

                while (!cliProcess.HasExited)
                {
                    if (cliProcess.TotalProcessorTime.Seconds > 10) // Don't wait longer than 10 seconds.
                    {
                        Process cli = Process.GetProcessById(cliProcess.Id);
                        if (!cli.HasExited)
                        {
                            cli.Kill();
                        }
                    }
                }

                if (success)
                {
                    userSettingService.SetUserSetting(UserSettingConstants.HandBrakeExeHash, base64Hash);
                }
            }
            catch (Exception e)
            {
                userSettingService.SetUserSetting(ASUserSettingConstants.HandBrakeBuild, 0);
                userSettingService.SetUserSetting(UserSettingConstants.HandBrakeExeHash, string.Empty);

                errorService.ShowError(
                    "Unable to Initialise HandBrake. This error is unrecoverable.", " Try restarting.", e);
            }
        }
    }
}
