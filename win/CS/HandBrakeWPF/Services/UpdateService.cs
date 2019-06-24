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
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Reflection;
    using System.Security.Cryptography;
    using System.Threading;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

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
            if (UwpDetect.IsUWP())
            {
                return; // Disable Update checker if we are in a UWP container.
            }

            if (Portable.IsPortable() && !Portable.IsUpdateCheckEnabled())
            {
                return; // Disable Update Check for Portable Mode.
            }

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
                        // Figure out which appcast we want to read.
                        string url = Constants.Appcast64;

                        if (VersionHelper.IsNightly())
                        {
                            url = Constants.AppcastUnstable64;
                        }

                        var currentBuild = VersionHelper.Build;

                        // Fetch the Appcast from our server.
                        HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
                        request.AllowAutoRedirect = false; // We will never do this.
                        request.UserAgent = string.Format("HandBrake Win Upd {0}", VersionHelper.GetVersionShort());
                        WebResponse response = request.GetResponse();

                        // Parse the data with the AppcastReader
                        var reader = new AppcastReader();
                        reader.GetUpdateInfo(new StreamReader(response.GetResponseStream()).ReadToEnd());

                        // Further parse the information
                        string build = reader.Build;
                        int latest = int.Parse(build);
                        int current = currentBuild;

                        // Security Check
                        // Verify the download URL is for handbrake.fr and served over https.
                        // This prevents a compromised appcast download tricking the GUI into downloading a file, or accessing another website or local network resource.
                        Uri uriResult;
                        bool result = Uri.TryCreate(reader.DownloadFile, UriKind.Absolute, out uriResult) && uriResult.Scheme == Uri.UriSchemeHttps;
                        if (!result || (uriResult.Host != "handbrake.fr" && uriResult.Host != "download.handbrake.fr"))
                        {
                            callback(new UpdateCheckInformation { NewVersionAvailable = false, Error = new Exception("The HandBrake update service is currently unavailable.") });
                            return;
                        }

                        // Validate the URL from the appcast is ours.
                        var info2 = new UpdateCheckInformation
                            {
                                NewVersionAvailable = latest > current,
                                DescriptionUrl = reader.DescriptionUrl,
                                DownloadFile = reader.DownloadFile,
                                Build = reader.Build,
                                Version = reader.Version,
                                Signature = reader.Hash
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
        /// <param name="expectedSignature">
        /// The expected DSA SHA265 Signature
        /// </param>
        /// <param name="completed">
        /// The complete.
        /// </param>
        /// <param name="progress">
        /// The progress.
        /// </param>
        public void DownloadFile(string url, string expectedSignature, Action<DownloadStatus> completed, Action<DownloadStatus> progress)
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
                       webRequest.UserAgent = string.Format("HandBrake Win Upd {0}", VersionHelper.GetVersionShort());
                       HttpWebResponse webResponse = (HttpWebResponse)webRequest.GetResponse();
                       long fileSize = webResponse.ContentLength;

                       Stream responceStream = wcDownload.OpenRead(url);
                       Stream localStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

                       int bytesSize;
                       byte[] downBuffer = new byte[2048];

                       while ((bytesSize = responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                       {
                           localStream.Write(downBuffer, 0, bytesSize);
                           progress(new DownloadStatus { BytesRead = localStream.Length, TotalBytes = fileSize });
                       }

                       responceStream.Close();
                       localStream.Close();

                       completed(
                           this.VerifyDownload(expectedSignature, tempPath)
                               ? new DownloadStatus { WasSuccessful = true, Message = "Download Complete." } :
                                 new DownloadStatus
                                   {
                                       WasSuccessful = false,
                                       Message = "Download Failed.  Checksum Failed. Please visit the website to download this update."
                                   });
                   }
                   catch (Exception exc)
                   {
                       progress(new DownloadStatus { WasSuccessful = false, Exception = exc, Message = "Download Failed. Please visit the website to download this update." });
                   }
               });
        }

        /// <summary>
        /// Verify the HandBrake download is Valid.
        /// </summary>
        /// <param name="signature">The DSA SHA256 Signature from the appcast</param>
        /// <param name="updateFile">Path to the downloaded update file</param>
        /// <returns>True if the file is valid, false otherwise.</returns>
        public bool VerifyDownload(string signature, string updateFile)
        {
            // Sanity Checks
            if (!File.Exists(updateFile))
            {
                return false;
            }

            if (string.IsNullOrEmpty(signature))
            {
                return false;
            }

            // Fetch our Public Key
            string publicKey;
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("HandBrakeWPF.public.key"))
            {
                if (stream == null)
                {
                    return false;
                }

                using (StreamReader reader = new StreamReader(stream))
                {
                    publicKey = reader.ReadToEnd();
                }
            }
            
            // Verify the file against the Signature. 
            try
            {
                byte[] file = File.ReadAllBytes(updateFile);
                using (RSACryptoServiceProvider verifyProfider = new RSACryptoServiceProvider())
                {
                    verifyProfider.FromXmlString(publicKey);
                    return verifyProfider.VerifyData(file, "SHA256", Convert.FromBase64String(signature));
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e);
                return false;
            }
        }

        #endregion
    }
}