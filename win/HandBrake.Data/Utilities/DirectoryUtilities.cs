// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DirectoryUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DirectoryUtilities type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using System;
    using System.IO;
    using HandBrake.EventArgs;
    using HandBrake.Model.Prompts;
    using HandBrake.Properties;
    using HandBrake.Services.Interfaces;
    using PlatformBindings;
    using PlatformBindings.Enums;

    /// <summary>
    /// The directory utilities.
    /// </summary>
    public class DirectoryUtilities
    {
        /// <summary>
        /// Event fired when fetching the Storage Directory.
        /// </summary>
        public static event EventHandler<DirectoryFetchingEventArgs> GettingStorageDirectory;

        /// <summary>
        /// The get user storage path.
        /// </summary>
        /// <param name="isNightly">
        /// The is nightly.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetUserStoragePath(bool isNightly)
        {
            if (isNightly)
            {
                return Path.Combine(GetStorageDirectory(), "HandBrake", "Nightly");
            }
            else
            {
                return Path.Combine(GetStorageDirectory(), "HandBrake");
            }
        }

        /// <summary>
        /// Get the app default log directory.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetLogDirectory()
        {
            return Path.Combine(GetStorageDirectory(), "HandBrake", "logs");
        }

        /// <summary>
        /// Simple way of checking if a directory is writeable.
        /// </summary>
        /// <param name="dirPath">Path to check</param>
        /// <param name="createDirectoryPrompt">
        /// Prompt to create directory if it doesn't exist.
        /// </param>
        /// <param name="errorService">
        /// An instance of the error service to allow prompting.
        /// </param>
        /// <returns>True if writable</returns>
        public static bool IsWritable(string dirPath, bool createDirectoryPrompt, IErrorService errorService)
        {
            try
            {
                if (!Directory.Exists(dirPath))
                {
                    var result = errorService.ShowMessageBox(string.Format(Resources.DirectoryUtils_CreateFolderMsg, dirPath), Resources.DirectoryUtils_CreateFolder, DialogButtonType.YesNo, DialogType.Question);
                    if (result == Model.Prompts.DialogResult.Yes)
                    {
                        Directory.CreateDirectory(dirPath);
                    }
                }

                using (File.Create(Path.Combine(dirPath, Path.GetRandomFileName()), 1, FileOptions.DeleteOnClose))
                {
                }

                return true;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// The get storage directory.
        /// </summary>
        /// <returns>
        /// The storage directory. Either AppData or portable location.
        /// </returns>
        private static string GetStorageDirectory()
        {
            var args = new DirectoryFetchingEventArgs();
            if (AppServices.Current.ServicePlatform == Platform.Win32)
            {
                args.Directory = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            }
            else
            {
                args.Directory = AppServices.Current.IO.GetBaseFolder(PathRoot.LocalAppStorage).Path;
            }

            GettingStorageDirectory?.Invoke(null, args);

            return args.Directory;
        }
    }
}