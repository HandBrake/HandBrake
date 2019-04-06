// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ApiRouter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   This is a service worker for the HandBrake app. It allows us to run encodes / scans in a seperate process easily.
//   All API's expose the ApplicationServices models as JSON.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker
{
    using System.IO;
    using System.Net;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;

    using Newtonsoft.Json;

    public class ApiRouter
    {
        private readonly JsonSerializerSettings jsonNetSettings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
        private HandBrakeInstance handbrakeInstance;

        public string Initialise(HttpListenerRequest request)
        {
            if (this.handbrakeInstance == null)
            {
                this.handbrakeInstance = new HandBrakeInstance();
            }

            // TODO support verbosity
            this.handbrakeInstance.Initialize(1, true); // TODO enable user setting support for nohardware

            return null;
        }

        public string GetVersionInfo(HttpListenerRequest request)
        {
            string versionInfo = JsonConvert.SerializeObject(VersionHelper.GetVersion(), Formatting.Indented, this.jsonNetSettings);

            return versionInfo;
        }

        public string StartEncode(HttpListenerRequest request)
        {
            string requestPostData = GetRequestPostData(request);

            this.handbrakeInstance.StartEncode(requestPostData);

            return null;
        }

        public string StopEncode(HttpListenerRequest request)
        {
            this.handbrakeInstance?.StopEncode();

            return (string)null;
        }

        public string PauseEncode(HttpListenerRequest request)
        {
            this.handbrakeInstance?.PauseEncode();

            return null;
        }

        public string ResumeEncode(HttpListenerRequest request)
        {
            this.handbrakeInstance?.ResumeEncode();

            return null;
        }

        public string PollEncodeProgress(HttpListenerRequest request)
        {
            if (this.handbrakeInstance != null)
            {
                JsonState statusJson = this.handbrakeInstance.GetEncodeProgress();
                string versionInfo = JsonConvert.SerializeObject(statusJson, Formatting.Indented, this.jsonNetSettings);

                return versionInfo;
            }

            return null;
        }

        public string SetConfiguration(HttpListenerRequest request)
        {
            return null;
        }

        private static string GetRequestPostData(HttpListenerRequest request)
        {
            if (!request.HasEntityBody)
            {
                return null;
            }

            using (Stream inputStream = request.InputStream)
            {
                using (StreamReader streamReader = new StreamReader(inputStream, request.ContentEncoding))
                {
                    return streamReader.ReadToEnd();
                }
            }
        }
    }
}
