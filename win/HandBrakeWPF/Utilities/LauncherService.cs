// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LauncherService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using HandBrake.Model.Prompts;
    using HandBrake.Properties;
    using HandBrake.Services.Interfaces;

    public class LauncherService : LauncherServiceBase
    {
        public LauncherService(IErrorService errorService, IUserSettingService userSettingService)
            : base(errorService, userSettingService)
        {
        }

        public override void OpenDirectory(string directory)
        {
            if (!string.IsNullOrEmpty(directory) && Directory.Exists(directory))
            {
                Process.Start(directory);
            }
            else
            {
                var result =
                    ErrorService.ShowMessageBox(
                        string.Format(Resources.DirectoryUtils_CreateFolderMsg, directory),
                        Resources.DirectoryUtils_CreateFolder,
                        DialogButtonType.YesNo,
                        DialogType.Question);
                if (result == DialogResult.Yes)
                {
                    Directory.CreateDirectory(directory);
                    Process.Start(directory);
                }
            }
        }

        public override void PlayFile(string path)
        {
            string args = "\"" + path + "\"";

            if (this.UseSystemDefaultPlayer)
            {
                Process.Start(args);
            }
            else
            {
                if (!File.Exists(this.UserSettingService.GetUserSetting<string>(UserSettingConstants.VLCPath)))
                {
                    // Attempt to find VLC if it doesn't exist in the default set location.
                    string vlcPath;

                    if (IntPtr.Size == 8 || (!string.IsNullOrEmpty(Environment.GetEnvironmentVariable("PROCESSOR_ARCHITEW6432"))))
                        vlcPath = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
                    else
                        vlcPath = Environment.GetEnvironmentVariable("ProgramFiles");

                    if (!string.IsNullOrEmpty(vlcPath))
                    {
                        vlcPath = Path.Combine(vlcPath, "VideoLAN\\VLC\\vlc.exe");
                    }

                    if (File.Exists(vlcPath))
                    {
                        this.UserSettingService.SetUserSetting(UserSettingConstants.VLCPath, vlcPath);
                    }
                    else
                    {
                        this.ErrorService.ShowMessageBox(Resources.StaticPreviewViewModel_UnableToFindVLC,
                                                         Resources.Error, DialogButtonType.OK, DialogType.Warning);
                    }
                }

                if (File.Exists(this.UserSettingService.GetUserSetting<string>(UserSettingConstants.VLCPath)))
                {
                    ProcessStartInfo vlc = new ProcessStartInfo(this.UserSettingService.GetUserSetting<string>(UserSettingConstants.VLCPath), args);
                    Process.Start(vlc);
                }
            }
        }
    }
}