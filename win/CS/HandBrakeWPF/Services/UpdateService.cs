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
    using System.Net.Http;
    using System.Reflection;
    using System.Security.Cryptography;
    using System.Threading;
    using System.Threading.Tasks;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

    using AppcastReader = Utilities.AppcastReader;

    public class UpdateService : IUpdateService
    {
        private readonly IUserSettingService userSettingService;

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

        /// <summary>
        /// Perform an update check at application start, but only daily, weekly or monthly depending on the users settings.
        /// </summary>
        /// <param name="callback">
        /// The callback.
        /// </param>
        public void PerformStartupUpdateCheck(Action<UpdateCheckInformation> callback)
        {
            if (Portable.IsPortable() && !Portable.IsUpdateCheckEnabled())
            {
                return; // Disable Update Check for Portable Mode.
            }

            // Make sure it's running on the calling thread
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus))
            {
                // If a previous update check detected an update, don't bother calling out to the HandBrake website again. Just return the result. 
                int lastLatestBuildNumberCheck = this.userSettingService.GetUserSetting<int>(UserSettingConstants.IsUpdateAvailableBuild);
                if (lastLatestBuildNumberCheck != 0 && lastLatestBuildNumberCheck > HandBrakeVersionHelper.Build)
                {
                    callback(new UpdateCheckInformation { NewVersionAvailable = true, Error = null });
                    return;
                }

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
                        string url = SystemInfo.IsArmDevice ? Constants.Appcast64Arm : Constants.Appcast64;

                        if (HandBrakeVersionHelper.IsNightly())
                        {
                            url = SystemInfo.IsArmDevice ? Constants.AppcastUnstable64Arm : Constants.AppcastUnstable64;
                        }

                        // Fetch the Appcast from our server.
                        string appcastContent = Task.Run(() => GetHttpContent(url)).GetAwaiter().GetResult();
                        
                        // Parse the data with the AppcastReader
                        var reader = new AppcastReader();
                        reader.GetUpdateInfo(appcastContent);

                        // Further parse the information
                        string build = reader.Build;

                        int latest = 0;
                        if (!int.TryParse(build, out latest))
                        {
                            throw new Exception("Build Information not available");
                        }

                        // Security Check
                        // Verify the download URL is for handbrake.fr and served over https.
                        // This prevents a compromised appcast download tricking the GUI into downloading a file, or accessing another website or local network resource.
                        // The download itself will also be checked against a signature later. 
                        Uri uriResult;
                        bool result = Uri.TryCreate(reader.DownloadFile, UriKind.Absolute, out uriResult) && uriResult.Scheme == Uri.UriSchemeHttps;

                        bool isHandBrakeGitHub = uriResult.Host == "github.com" && uriResult.AbsolutePath.StartsWith("/HandBrake/", StringComparison.OrdinalIgnoreCase);
                        if (!result || (uriResult.Host != "handbrake.fr" && uriResult.Host != "download.handbrake.fr" && !isHandBrakeGitHub))
                        {
                            this.userSettingService.SetUserSetting(UserSettingConstants.IsUpdateAvailableBuild, 0);
                            callback(new UpdateCheckInformation { NewVersionAvailable = false, Error = new Exception("The HandBrake update service is currently unavailable.") });
                            return;
                        }

                        // Validate the URL from the appcast is ours.
                        var info2 = new UpdateCheckInformation
                            {
                                NewVersionAvailable = latest > HandBrakeVersionHelper.Build,
                                DescriptionUrl = reader.DescriptionUrl,
                                DownloadFile = reader.DownloadFile,
                                Build = reader.Build,
                                Version = reader.Version,
                                Signature = reader.Signature,
                            };

                        this.userSettingService.SetUserSetting(UserSettingConstants.IsUpdateAvailableBuild, latest);

                        callback(info2);
                    }
                    catch (Exception exc)
                    {
                        this.userSettingService.SetUserSetting(UserSettingConstants.IsUpdateAvailableBuild, 0);
                        callback(new UpdateCheckInformation { NewVersionAvailable = false, Error = exc });
                    }
                });
        }

        /// <summary>
        /// Download the update file.
        /// </summary>
        /// <param name="update">
        /// Update Check Information
        /// </param>
        /// <param name="completed">
        /// The complete.
        /// </param>
        /// <param name="progress">
        /// The progress.
        /// </param>
        public void DownloadFile(UpdateCheckInformation update, Action<DownloadStatus> completed, Action<DownloadStatus> progress)
        {
            ThreadPool.QueueUserWorkItem(
               delegate
               {
                   try
                   {
                       string tempPath = Path.Combine(Path.GetTempPath(), "handbrake-setup.exe");

                       Task.Run(() => DownloadSetupFile(update.DownloadFile, progress, tempPath)).GetAwaiter().GetResult();

                       completed(
                           this.VerifyDownload(update.Signature, tempPath)
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
        /// <param name="signature">The RSA SHA256 Signature from the appcast</param>
        /// <param name="updateFile">Path to the downloaded update file</param>
        /// <returns>True if the file is valid, false otherwise.</returns>
        public bool VerifyDownload(string signature, string updateFile)
        {
            if (!File.Exists(updateFile))
            {
                return false;
            }

            if (string.IsNullOrEmpty(signature))
            {
                return false;
            }

            // Fetch our Public Key
            string publicKey = GetPublicKey( "HandBrakeWPF.public.4096.key");
            if (string.IsNullOrEmpty(publicKey))
            {
                return false;
            }

            try
            {
                byte[] file = File.ReadAllBytes(updateFile);
                using (RSA rsa = RSA.Create())
                {
                    rsa.FromXmlString(publicKey);
                    return rsa.VerifyData(file, Convert.FromBase64String(signature), HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1);
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e);
                return false;
            }
        }

        private async Task<string> GetHttpContent(string url)
        {
            using (var httpClient = new HttpClient())
            {
                string armDevice = SystemInfo.IsArmDevice ? "_ARM" : string.Empty;
                httpClient.DefaultRequestHeaders.Add("User-Agent", string.Format("HandBrakeWinUpdate{0} {1}", armDevice, HandBrakeVersionHelper.Version));
                httpClient.MaxResponseContentBufferSize = 1 * 1024 * 1024; // 1 MB cap
                httpClient.Timeout = TimeSpan.FromSeconds(30);

                var httpResponse = await httpClient.GetAsync(url);
                httpResponse.EnsureSuccessStatusCode();

                var contents = await httpResponse.Content.ReadAsStringAsync();

                return contents;
            }
        }

        private async Task<bool> DownloadSetupFile(string url, Action<DownloadStatus> progress, string tempPath)
        {
            if (File.Exists(tempPath))
            {
                File.Delete(tempPath);
            }

            using (HttpClient httpClient = new HttpClient())
            {
                string armDevice = SystemInfo.IsArmDevice ? "_ARM" : string.Empty;

                httpClient.DefaultRequestHeaders.Add("User-Agent", string.Format("HandBrakeWinUpdate{0} {1}", armDevice, HandBrakeVersionHelper.Version));

                using (HttpResponseMessage httpResponse = await httpClient.GetAsync(new Uri(url), HttpCompletionOption.ResponseHeadersRead))
                {
                    httpResponse.EnsureSuccessStatusCode();

                    var contentLength = httpResponse.Content.Headers.ContentLength.HasValue ? httpResponse.Content.Headers.ContentLength.Value : -1L;
                    using (Stream contentStream = await httpResponse.Content.ReadAsStreamAsync(), fileStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.ReadWrite, 8192, true))
                    {
                        var buffer = new byte[8192];
                        var totalRead = 0L;
                        var totalReads = 0L;
                        var isMoreToRead = true;

                        do
                        {
                            var read = await contentStream.ReadAsync(buffer, 0, buffer.Length);
                            if (read == 0)
                            {
                                isMoreToRead = false;
                            }
                            else
                            {
                                await fileStream.WriteAsync(buffer, 0, read);

                                totalRead += read;
                                totalReads += 1;

                                if (totalReads % 100 == 0)
                                {
                                    progress(new DownloadStatus { BytesRead = totalRead, TotalBytes = contentLength });
                                }
                            }
                        }
                        while (isMoreToRead);
                    }
                }
            }

            return true;
        }

        private string GetPublicKey(string keyFile)
        {
            string publicKey;
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(keyFile))
            {
                if (stream == null)
                {
                    return null;
                }

                using (StreamReader reader = new StreamReader(stream))
                {
                    publicKey = reader.ReadToEnd();
                }
            }

            return publicKey;
        }
    }
}