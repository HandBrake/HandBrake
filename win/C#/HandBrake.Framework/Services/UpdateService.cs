/*  UpdateService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Services
{
    using System;
    using System.IO;
    using System.Net;
    using System.Threading;

    using HandBrake.Framework.Model;
    using HandBrake.Framework.Services.Interfaces;

    public class UpdateService
    {
        /// <summary>
        /// Begins checking for an update to HandBrake.
        /// </summary>
        /// <param name="callback">
        /// The method that will be called when the check is finished.
        /// </param>
        /// <param name="debug">
        /// Whether or not to execute this in debug mode.
        /// </param>
        /// <param name="url">
        /// The url.
        /// </param>
        /// <param name="currentBuild">
        /// The current Build.
        /// </param>
        /// <param name="skipBuild">
        /// The skip Build.
        /// </param>
        /// <param name="currentVersion">
        /// The current Version.
        /// </param>
        public static void BeginCheckForUpdates(AsyncCallback callback, bool debug, string url, int currentBuild, int skipBuild, string currentVersion)
        {
            ThreadPool.QueueUserWorkItem(new WaitCallback(delegate
            {
                try
                {
                    // Initialize variables
                    WebRequest request = WebRequest.Create(url);
                    WebResponse response = request.GetResponse();
                    IAppcastReader reader = new AppcastReader();

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
                        UpdateCheckInformation info =
                            new UpdateCheckInformation
                            {
                                NewVersionAvailable = false,
                                BuildInformation = null
                            };
                        callback(new UpdateCheckResult(debug, info));
                        return;
                    }

                    UpdateCheckInformation info2 = new UpdateCheckInformation
                        {
                            NewVersionAvailable = latest > current,
                            BuildInformation = reader
                        };
                    callback(new UpdateCheckResult(debug, info2));
                }
                catch (Exception exc)
                {
                    callback(new UpdateCheckResult(debug, new UpdateCheckInformation { Error = exc }));
                }
            }));
        }

        /// <summary>
        /// End Check for Updates
        /// </summary>
        /// <param name="result">
        /// The result.
        /// </param>
        /// <returns>
        /// Update Check information
        /// </returns>
        public static UpdateCheckInformation EndCheckForUpdates(IAsyncResult result)
        {
            return ((UpdateCheckResult)result).Result;
        }
    }
}
