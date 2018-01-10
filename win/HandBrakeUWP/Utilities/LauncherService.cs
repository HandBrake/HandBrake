// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LauncherService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using System;
    using HandBrake.Services.Interfaces;
    using Windows.Storage;
    using Windows.System;

    public class LauncherService : LauncherServiceBase
    {
        public LauncherService(IErrorService errorService, IUserSettingService userSettingService)
    : base(errorService, userSettingService)
        {
        }

        public override async void OpenDirectory(string path)
        {
            var folder = await StorageFolder.GetFolderFromPathAsync(path);
            await Launcher.LaunchFolderAsync(folder);
        }

        public override async void PlayFile(string path)
        {
            var file = await StorageFile.GetFileFromPathAsync(path);

            var options = new LauncherOptions();
            if (!this.UseSystemDefaultPlayer)
            {
                // Prefer VLC.
                options.PreferredApplicationPackageFamilyName = "VideoLAN.VLC_paz6r1rewnh0a";
                options.PreferredApplicationDisplayName = "VLC";
            }

            await Launcher.LaunchFileAsync(file, options);
        }
    }
}