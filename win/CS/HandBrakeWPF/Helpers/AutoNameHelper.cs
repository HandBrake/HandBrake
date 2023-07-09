// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AutoNameHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AutoNameHelper type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.IO;
    using System.Linq;

    using HandBrake.App.Core.Extensions;

    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;
    using VideoEncodeRateType = Model.Video.VideoEncodeRateType;

    /// <summary>
    /// The Destination AutoName Helper
    /// </summary>
    public class AutoNameHelper
    {
        /// <summary>
        /// Function which generates the filename and path automatically based on 
        /// the Source Name, DVD title and DVD Chapters
        /// </summary>
        public static string AutoName(EncodeTask task, string titleName, string sourceDisplayName, Preset presetName)
        {
            IUserSettingService userSettingService = IoCHelper.Get<IUserSettingService>();
            if (task.Destination == null)
            {
                task.Destination = string.Empty;
            }

            string sourceOrLabelName = !string.IsNullOrEmpty(titleName) ? titleName : sourceDisplayName;

            if (task.Title != 0)
            {
                // Get the Source Name and remove any invalid characters and clean it per users options.
                string sourceName = Path.GetInvalidFileNameChars().Aggregate(sourceOrLabelName, (current, character) => current.Replace(character.ToString(), string.Empty));
                sourceName = CleanupSourceName(sourceName, userSettingService);

                // Get the Selected Title Number
                string dvdTitle = task.Title.ToString();

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = task.StartPoint.ToString();
                string chapterFinish = task.EndPoint.ToString();
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != string.Empty)
                {
                    combinedChapterTag = chapterStart + "-" + chapterFinish;
                }

                /*
                 * Generate the full path and filename
                 */
                string destinationFilename = GenerateDestinationFileName(task, userSettingService, sourceName, dvdTitle, combinedChapterTag, presetName);
                string autoNamePath = GetAutonamePath(userSettingService, task, sourceName);
                string finalPath = Path.Combine(autoNamePath, destinationFilename);

                autoNamePath = CheckAndHandleFilenameCollisions(finalPath, destinationFilename, task, userSettingService);
                return autoNamePath;
            }

            return string.Empty;
        }
        
        /// <summary>
        /// Check if there is a valid autoname path.
        /// </summary>
        /// <returns>
        /// True if there is a valid path
        /// </returns>
        public static bool IsAutonamingEnabled()
        {
            IUserSettingService userSettingService = IoCHelper.Get<IUserSettingService>();

            if (!userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
            {
                return false;
            }

            if (string.IsNullOrEmpty(
                    userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim()))
            {
                return false;
            }

            return true;
        }

        private static string CleanupSourceName(string sourceName, IUserSettingService userSettingService)
        {
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore))
            {
                sourceName = sourceName.Replace("_", " ");
            }

            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.RemovePunctuation))
            {
                sourceName = sourceName.Replace("-", string.Empty);
                sourceName = sourceName.Replace(",", string.Empty);
                sourceName = sourceName.Replace(".", string.Empty);
            }

            // Switch to "Title Case"
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase))
            {
                sourceName = sourceName.ToTitleCase();
            }

            return sourceName;
        }

        private static string GenerateDestinationFileName(EncodeTask task, IUserSettingService userSettingService, string sourceName, string dvdTitle, string combinedChapterTag, Preset presetName)
        {
            string destinationFilename;
            if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != string.Empty)
            {
                string presetNameStr = presetName?.Name ?? string.Empty;
                presetNameStr = Path.GetInvalidFileNameChars().Aggregate(
                    presetNameStr,
                    (current, character) => current.Replace(character.ToString(), string.Empty));

                int? bitDepth = task.VideoEncoder?.BitDepth;
                
                // Creation Date / Time
                var creationDateTime = ObtainCreateDateObject(task);
                string createDate = userSettingService.GetUserSetting<bool>(UserSettingConstants.UseIsoDateFormat) ? creationDateTime.Date.ToString("yyyy-MM-dd") : creationDateTime.Date.ToShortDateString().Replace('/', '-');
               
                string createTime = creationDateTime.ToString("HH-mm");

                // Modification Date / Time
                var modificationDateTime = GetFileModificationDate(task);
                string modifyDate = userSettingService.GetUserSetting<bool>(UserSettingConstants.UseIsoDateFormat) ? modificationDateTime.Date.ToString("yyyy-MM-dd") : modificationDateTime.Date.ToShortDateString().Replace('/', '-');
                string modifyTime = modificationDateTime.ToString("HH-mm");

                destinationFilename = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);
                destinationFilename =
                    destinationFilename
                        .Replace(Constants.Source, sourceName)
                        .Replace(Constants.Title, dvdTitle)
                        .Replace(Constants.Chapters, combinedChapterTag)
                        .Replace(Constants.Date, DateTime.Now.Date.ToShortDateString().Replace('/', '-'))
                        .Replace(Constants.Time, DateTime.Now.ToString("HH-mm"))
                        .Replace(Constants.CreationDate, createDate)
                        .Replace(Constants.CreationTime, createTime)
                        .Replace(Constants.ModificationDate, modifyDate)
                        .Replace(Constants.ModificationTime, modifyTime)
                        .Replace(Constants.Preset, presetNameStr)
                        .Replace(Constants.EncoderBitDepth, bitDepth?.ToString())
                        .Replace(Constants.StorageWidth, task.Width?.ToString())
                        .Replace(Constants.StorageHeight, task.Height?.ToString())
                        .Replace(Constants.Codec, task.VideoEncoder?.Codec)
                        .Replace(Constants.EncoderDisplay, task.VideoEncoder?.DisplayName)
                        .Replace(Constants.Encoder, task.VideoEncoder?.ShortName);


                if (task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality)
                {
                    destinationFilename = destinationFilename.Replace(Constants.QualityBitrate, task.Quality.ToString());
                    destinationFilename = destinationFilename.Replace(Constants.QualityType, "Q");
                }
                else
                {
                    destinationFilename = destinationFilename.Replace(Constants.QualityBitrate, task.VideoBitrate.ToString());
                    destinationFilename = destinationFilename.Replace(Constants.QualityType, "kbps");
                }
            }
            else
            {
                destinationFilename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;
            }

            /*
             * File Extension
             */
            if (task.OutputFormat == OutputFormat.Mp4)
            {
                switch ((Mp4Behaviour)userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                {
                    case Mp4Behaviour.MP4: // Always MP4
                        destinationFilename += ".mp4";
                        break;
                    case Mp4Behaviour.M4V: // Always M4V
                        destinationFilename += ".m4v";
                        break;
                }
            }
            else if (task.OutputFormat == OutputFormat.Mkv)
            {
                destinationFilename += ".mkv";
            }
            else if (task.OutputFormat == OutputFormat.WebM)
            {
                destinationFilename += ".webm";
            }

            return destinationFilename;
        }

        private static string GetAutonamePath(IUserSettingService userSettingService, EncodeTask task, string sourceName)
        {
            string autoNamePath = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().Replace("/", "\\");

            // If enabled, use the current Destination path.
            if (!userSettingService.GetUserSetting<bool>(UserSettingConstants.AlwaysUseDefaultPath) && !string.IsNullOrEmpty(task.Destination))
            {
                string path = Path.GetDirectoryName(task.Destination);
                if (!string.IsNullOrEmpty(path))
                {
                    return path;
                }
            }

            // Handle {source_path} 
            if (autoNamePath.StartsWith(Constants.SourcePath) && !string.IsNullOrEmpty(task.Source))
            {
                string savedPath = autoNamePath.Replace(Constants.SourcePath + "\\", string.Empty).Replace(Constants.SourcePath, string.Empty);
                string directory = Directory.Exists(task.Source) ? task.Source : Path.GetDirectoryName(task.Source);
                autoNamePath = Path.Combine(directory, savedPath);
            }

            // Handle {source}
            if (autoNamePath.Contains(Constants.Source) && !string.IsNullOrEmpty(task.Source))
            {
                sourceName = Path.GetInvalidPathChars().Aggregate(sourceName, (current, character) => current.Replace(character.ToString(), string.Empty));
                autoNamePath = autoNamePath.Replace(Constants.Source, sourceName);
            }

            // Handle {source_folder_name}
            if (autoNamePath.Contains(Constants.SourceFolderName) && !string.IsNullOrEmpty(task.Source))
            {
                // Second Case: We have a Path, with "{source_folder}" in it, therefore we need to replace it with the folder name from the source.
                string path = Path.GetDirectoryName(task.Source);
                if (!string.IsNullOrEmpty(path))
                {
                    string[] filesArray = path.Split(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                    string sourceFolder = filesArray[filesArray.Length - 1];

                    autoNamePath = autoNamePath.Replace(Constants.SourceFolderName, sourceFolder);
                }
            }

            // Fallback to the users "Videos" folder.
            if (string.IsNullOrEmpty(autoNamePath) || autoNamePath == Resources.OptionsView_SetDefaultLocationOutputFIle)
            {
                autoNamePath = Environment.GetFolderPath(Environment.SpecialFolder.MyVideos);
            }
            
            return autoNamePath;
        }

        private static string CheckAndHandleFilenameCollisions(string autoNamePath, string destinationFilename, EncodeTask task, IUserSettingService userSettingService)
        {
            AutonameFileCollisionBehaviour behaviour = (AutonameFileCollisionBehaviour)userSettingService.GetUserSetting<int>(UserSettingConstants.AutonameFileCollisionBehaviour);
            string prefix = string.Empty, postfix = string.Empty;
            switch (behaviour)
            {
                case AutonameFileCollisionBehaviour.Postfix:
                    postfix = userSettingService.GetUserSetting<string>(UserSettingConstants.AutonameFilePrePostString);
                    break;
                case AutonameFileCollisionBehaviour.Prefix:
                    prefix = userSettingService.GetUserSetting<string>(UserSettingConstants.AutonameFilePrePostString);
                    break;
            }

            string extension = Path.GetExtension(destinationFilename);
            string filenameWithoutExt = Path.GetFileNameWithoutExtension(destinationFilename);

            if (behaviour != AutonameFileCollisionBehaviour.AppendNumber)
            {
                autoNamePath = Path.Combine(Path.GetDirectoryName(autoNamePath), prefix + filenameWithoutExt + postfix + extension);

                int counter = 0;
                while (File.Exists(autoNamePath))
                {
                    counter = counter + 1;
                    string appendedNumber = string.Format("({0})", counter);
                    autoNamePath = Path.Combine(Path.GetDirectoryName(autoNamePath), prefix + filenameWithoutExt + postfix + appendedNumber + extension);
                }
            }
            else
            {
                int counter = 0;
                while (File.Exists(autoNamePath))
                {
                    counter = counter + 1;
                    string appendedNumber = string.Format("({0})", counter);
                    autoNamePath = Path.Combine(Path.GetDirectoryName(autoNamePath),  filenameWithoutExt + appendedNumber + extension);
                }
            }

            return autoNamePath;
        }

        private static DateTime ObtainCreateDateObject(EncodeTask task)
        {
            var rd = task.MetaData.ReleaseDate;
            if (DateTime.TryParse(rd, out var d))
            {
                return d;
            }

            try
            {
                return File.GetCreationTime(task.Source);
            }
            catch (Exception e)
            {
                if (e is UnauthorizedAccessException || e is PathTooLongException || e is NotSupportedException)
                {
                    // Suspect the most likely concerns trying to grab the creation date in which we would want to swallow exception.
                    return default(DateTime);
                }

                throw;
            }
        }

        private static DateTime GetFileModificationDate(EncodeTask task)
        {
            var rd = task.MetaData.ReleaseDate;
            if (DateTime.TryParse(rd, out var d))
            {
                return d;
            }

            try
            {
                return File.GetLastWriteTime(task.Source);
            }
            catch (Exception e)
            {
                if (e is UnauthorizedAccessException || e is PathTooLongException || e is NotSupportedException)
                {
                    // Suspect the most likely concerns trying to grab the creation date in which we would want to swallow exception.
                    return default(DateTime);
                }

                throw;
            }
        }
    }
}
