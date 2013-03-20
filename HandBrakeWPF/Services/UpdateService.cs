// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UpdateService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Update Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.IO;
    using System.Net;
    using System.Threading;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// The Update Service
    /// </summary>
    public class UpdateService : IUpdateService
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for the update service
        /// </summary>
        private readonly IUserSettingService userSettingService;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="UpdateService"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user setting service.
        /// </param>
        public UpdateService(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Perform an update check at application start, but only daily, weekly or monthly depending on the users settings.
        /// </summary>
        /// <param name="callback">
        /// The callback.
        /// </param>
        public void PerformStartupUpdateCheck(Action<UpdateCheckInformation> callback)
        {
            // Make sure it's running on the calling thread
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus))
            {
                if (DateTime.Now.Subtract(this.userSettingService.GetUserSetting<DateTime>(UserSettingConstants.LastUpdateCheckDate)).TotalDays
                    > this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck))
                {
                    this.userSettingService.SetUserSetting(UserSettingConstants.LastUpdateCheckDate, DateTime.Now);

                    this.CheckForUpdates(callback);
                }
            }
        }

        /// <summary>
        /// Check for Updates
        /// </summary>
        /// <param name="callback">
        /// The callback.
        /// </param>
        public void CheckForUpdates(Action<UpdateCheckInformation> callback)
        {
            ThreadPool.QueueUserWorkItem(
                delegate
                {
                    try
                    {
                        string url =
                            VersionHelper.Is64Bit()
                                ? Constants.Appcast64
                                : Constants.Appcast32;

                        var currentBuild =
                            this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.HandBrakeBuild);
                        var skipBuild = this.userSettingService.GetUserSetting<int>(
                            UserSettingConstants.Skipversion);

                        // Initialize variables
                        WebRequest request = WebRequest.Create(url);
                        WebResponse response = request.GetResponse();
                        var reader = new AppcastReader();

                        // Get the data, convert it to a string, and parse it into the AppcastReader
                        reader.GetUpdateInfo(new StreamReader(response.GetResponseStream()).ReadToEnd());

                        // Further parse the information
                        string build = reader.Build;

                        int latest = int.Parse(build);
                        int current = currentBuild;
                        int skip = skipBuild;

                        // If the user wanted to skip this version, don't report the update
                        if (latest == skip)
                        {
                            var info = new UpdateCheckInformation { NewVersionAvailable = false };
                            callback(info);
                            return;
                        }

                        var info2 = new UpdateCheckInformation
                            {
                                NewVersionAvailable = latest > current,
                                DescriptionUrl = reader.DescriptionUrl,
                                DownloadFile = reader.DownloadFile,
                                Build = reader.Build,
                                Version = reader.Version,
                            };

                        callback(info2);
                    }
                    catch (Exception exc)
                    {
                        callback(new UpdateCheckInformation { NewVersionAvailable = false, Error = exc });
                    }
                });
        }

        /// <summary>
        /// Download the update file.
        /// </summary>
        /// <param name="url">
        /// The url.
        /// </param>
        /// <param name="completed">
        /// The complete.
        /// </param>
        /// <param name="progress">
        /// The progress.
        /// </param>
        public void DownloadFile(string url, Action<DownloadStatus> completed, Action<DownloadStatus> progress)
        {
            ThreadPool.QueueUserWorkItem(
               delegate
               {
                   string tempPath = Path.Combine(Path.GetTempPath(), "handbrake-setup.exe");
                   WebClient wcDownload = new WebClient();

                   try
                   {
                       if (File.Exists(tempPath))
                           File.Delete(tempPath);

                       HttpWebRequest webRequest = (HttpWebRequest)WebRequest.Create(url);
                       webRequest.Credentials = CredentialCache.DefaultCredentials;
                       HttpWebResponse webResponse = (HttpWebResponse)webRequest.GetResponse();
                       long fileSize = webResponse.ContentLength;

                       Stream responceStream = wcDownload.OpenRead(url);
                       Stream localStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

                       int bytesSize;
                       byte[] downBuffer = new byte[2048];

                       long flength = 0;
                       while ((bytesSize = responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                       {
                           localStream.Write(downBuffer, 0, bytesSize);
                           flength = localStream.Length;
                           progress(new DownloadStatus { BytesRead = localStream.Length, TotalBytes = fileSize});
                       }

                       responceStream.Close();
                       localStream.Close();

                       completed(
                           flength != fileSize
                               ? new DownloadStatus
                                   {
                                       WasSuccessful = false,
                                       Message = "Partial Download. File is Incomplete. Please Retry the download."
                                   }
                               : new DownloadStatus { WasSuccessful = true, Message = "Download Complete." });
                   }
                   catch (Exception exc)
                   {
                       progress(new DownloadStatus { WasSuccessful = false, Exception = exc, Message = "Download Failed." });
                   }
               });
        }

        #endregion
    }
}