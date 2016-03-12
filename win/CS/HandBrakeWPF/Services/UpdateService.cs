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
    using System.Security.Cryptography;
    using System.Text;
    using System.Threading;

    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.ApplicationServices.Interop;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;

    using AppcastReader = HandBrakeWPF.Utilities.AppcastReader;

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
                DateTime lastUpdateCheck = this.userSettingService.GetUserSetting<DateTime>(UserSettingConstants.LastUpdateCheckDate);
                int checkFrequency = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck) == 0 ? 7 : 30;

                if (DateTime.Now.Subtract(lastUpdateCheck).TotalDays > checkFrequency)
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
                            VersionHelper.Is64Bit() || Environment.Is64BitOperatingSystem
                                ? Constants.Appcast64
                                : Constants.Appcast32;

                        if (VersionHelper.IsNightly())
                        {
                            url =
                            VersionHelper.Is64Bit() || Environment.Is64BitOperatingSystem
                                ? Constants.AppcastUnstable64
                                : Constants.AppcastUnstable32;
                        }

                        var currentBuild = HandBrakeUtils.Build;

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

                        var info2 = new UpdateCheckInformation
                            {
                                NewVersionAvailable = latest > current,
                                DescriptionUrl = reader.DescriptionUrl,
                                DownloadFile = reader.DownloadFile,
                                Build = reader.Build,
                                Version = reader.Version,
                                ExpectedSHA1Hash = reader.Hash
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
        /// <param name="expectedSha1Hash">
        /// The expected Sha 1 Hash.
        /// </param>
        /// <param name="completed">
        /// The complete.
        /// </param>
        /// <param name="progress">
        /// The progress.
        /// </param>
        public void DownloadFile(string url, string expectedSha1Hash, Action<DownloadStatus> completed, Action<DownloadStatus> progress)
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

                       while ((bytesSize = responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                       {
                           localStream.Write(downBuffer, 0, bytesSize);
                           progress(new DownloadStatus { BytesRead = localStream.Length, TotalBytes = fileSize});
                       }

                       responceStream.Close();
                       localStream.Close();

                       completed(
                           GetSHA1(tempPath) != expectedSha1Hash
                               ? new DownloadStatus
                                   {
                                       WasSuccessful = false,
                                       Message = "Download Failed.  SHA1 Checksum Failed. Please visit the website to download this update."
                                   }
                               : new DownloadStatus { WasSuccessful = true, Message = "Download Complete." });
                   }
                   catch (Exception exc)
                   {
                       progress(new DownloadStatus { WasSuccessful = false, Exception = exc, Message = "Download Failed. Please visit the website to download this update." });
                   }
               });
        }

        /// <summary>
        /// The get sh a 1.
        /// </summary>
        /// <param name="fileName">
        /// The file name.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static String GetSHA1(String fileName)
        {
            FileStream file = new FileStream(fileName, FileMode.Open);
            SHA1 sha1 = new SHA1CryptoServiceProvider();
            byte[] retVal = sha1.ComputeHash(file);
            file.Close();

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < retVal.Length; i++)
            {
                sb.Append(retVal[i].ToString("x2"));
            }
            return sb.ToString();
        }

        #endregion
    }
}