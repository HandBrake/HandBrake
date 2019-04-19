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

    using Caliburn.Micro;

    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Extensions;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;

    /// <summary>
    /// The Destination AutoName Helper
    /// </summary>
    public class AutoNameHelper
    {
        /// <summary>
        /// Function which generates the filename and path automatically based on 
        /// the Source Name, DVD title and DVD Chapters
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="sourceOrLabelName">
        /// The Source or Label Name
        /// </param>
        /// <param name="presetName">
        /// The preset Name.
        /// </param>
        /// <returns>
        /// The Generated FileName
        /// </returns>
        public static string AutoName(EncodeTask task, string sourceOrLabelName, Preset presetName)
        {
            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();
            if (task.Destination == null)
            {
                task.Destination = string.Empty;
            }

            string autoNamePath = string.Empty;
            if (task.Title != 0)
            {
                // Get the Source Name and remove any invalid characters
                string sourceName = Path.GetInvalidFileNameChars().Aggregate(sourceOrLabelName, (current, character) => current.Replace(character.ToString(), string.Empty));

                // Remove Underscores
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore))
                    sourceName = sourceName.Replace("_", " ");

                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.RemovePunctuation))
                {
                    sourceName = sourceName.Replace("-", string.Empty);
                    sourceName = sourceName.Replace(",", string.Empty);
                    sourceName = sourceName.Replace(".", string.Empty); 
                }
                  
                // Switch to "Title Case"
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase))
                    sourceName = sourceName.ToTitleCase();

                // Get the Selected Title Number

                string dvdTitle = task.Title.ToString();

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = task.StartPoint.ToString();
                string chapterFinish = task.EndPoint.ToString();
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != string.Empty)
                    combinedChapterTag = chapterStart + "-" + chapterFinish;

                // Local method to check if we have a creation time in the meta data. If not, fall back to source file creation time.
                DateTime obtainCreateDateObject()
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
                        if (e is UnauthorizedAccessException ||
                            e is PathTooLongException ||
                            e is NotSupportedException)
                        {
                            // Suspect the most likely concerns trying to grab the creation date in which we would want to swallow exception.
                            return default(DateTime);
                        }
                        throw e;                            
                    }
                }

                var creationDateTime = obtainCreateDateObject();
                string createDate = creationDateTime.Date.ToShortDateString().Replace('/', '-');
                string createTime = creationDateTime.ToString("HH-mm"); 

                /*
                 * File Name
                 */
                string destinationFilename;
                if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != string.Empty)
                {
                    destinationFilename = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);
                    destinationFilename =
                        destinationFilename
                            .RegexReplace(Constants.Source, sourceName)
                            .RegexReplace(Constants.Title, dvdTitle)
                            .RegexReplace(Constants.Chapters, combinedChapterTag)
                            .RegexReplace(Constants.Date, DateTime.Now.Date.ToShortDateString().Replace('/', '-'))
                            .RegexReplace(Constants.Time, DateTime.Now.ToString("HH-mm"))
                            .RegexReplace(Constants.CretaionDate, createDate)
                            .RegexReplace(Constants.CreationTime, createTime);

                    if (task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality)
                    {
                        destinationFilename = destinationFilename.Replace(Constants.Quality, task.Quality.ToString());
                        destinationFilename = destinationFilename.Replace(Constants.Bitrate, string.Empty);
                    }
                    else
                    {
                        destinationFilename = destinationFilename.Replace(
                            Constants.Bitrate,
                            task.VideoBitrate.ToString());
                        destinationFilename = destinationFilename.Replace(Constants.Quality, string.Empty);
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
                    switch (userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v, typeof(int)))
                    {
                        case 0: // Automatic
                            destinationFilename += task.IncludeChapterMarkers || MP4Helper.RequiresM4v(task) ? ".m4v" : ".mp4";
                            break;
                        case 1: // Always MP4
                            destinationFilename += ".mp4";
                            break;
                        case 2: // Always M4V
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

                /*
                 * File Destination Path
                 */

                // If there is an auto name path, use it...
                if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().StartsWith("{source_path}") && !string.IsNullOrEmpty(task.Source))
                {
                    string savedPath = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().Replace("{source_path}\\", string.Empty).Replace("{source_path}", string.Empty);

                    string directory = Directory.Exists(task.Source)
                                           ? task.Source
                                           : Path.GetDirectoryName(task.Source);
                    string requestedPath = Path.Combine(directory, savedPath);

                    autoNamePath = Path.Combine(requestedPath, destinationFilename);
                }
                else if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Contains("{source_folder_name}") && !string.IsNullOrEmpty(task.Source))
                {
                    // Second Case: We have a Path, with "{source_folder}" in it, therefore we need to replace it with the folder name from the source.
                    string path = Path.GetDirectoryName(task.Source);
                    if (!string.IsNullOrEmpty(path))
                    {
                        string[] filesArray = path.Split(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                        string sourceFolder = filesArray[filesArray.Length - 1];

                        autoNamePath = Path.Combine(userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Replace("{source_folder_name}", sourceFolder), destinationFilename);
                    }
                }
                else if (!task.Destination.Contains(Path.DirectorySeparatorChar.ToString()))
                {
                    // Third case: If the destination box doesn't already contain a path, make one.
                    if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim() != string.Empty &&
                        userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim() != "Click 'Browse' to set the default location")
                    {
                        autoNamePath = Path.Combine(userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath), destinationFilename);
                    }
                    else 
                    {
                        // ...otherwise, output to the source directory
                        autoNamePath = null;
                    }
                }
                else
                {
                    // Otherwise, use the path that is already there.
                    // Use the path and change the file extension to match the previous destination
                    autoNamePath = Path.Combine(Path.GetDirectoryName(task.Destination), destinationFilename);
                }

                // Append out_ to files that already exist or is the source file
                if (autoNamePath?.ToLower() == task.Source?.ToLower())
                {
                    autoNamePath = Path.Combine(Path.GetDirectoryName(task.Source), "output_" + destinationFilename);

                    int counter = 1;
                    while (autoNamePath == task.Source)
                    {
                        autoNamePath = Path.Combine(Path.GetDirectoryName(task.Source), string.Format("output{0}_", counter) + destinationFilename);
                        counter = counter + 1;
                    }
                }
            }

            return autoNamePath;
        }
        
        /// <summary>
        /// Check if there is a valid autoname path.
        /// </summary>
        /// <returns>
        /// True if there is a valid path
        /// </returns>
        public static bool IsAutonamingEnabled()
        {
            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();

            if (!userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
            {
                return false;
            }

            // If there is an auto name path, use it...
            return userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().StartsWith(Constants.SourcePath) ||
                (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Contains(Constants.SourceFolderName) ||
                 Directory.Exists(userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim()));
        }        
    }
}
