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
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;
    using Newtonsoft.Json;
    using System;
    using System.IO;
    using System.Net;

    public class ApiRouter
    {
        private HandBrakeInstance handbrakeInstance;

        public string GetVersionInfo(HttpListenerRequest request)
        {
            return string.Format(JsonConvert.SerializeObject((object)VersionHelper.GetVersion(), Formatting.Indented, new JsonSerializerSettings()
            {
                NullValueHandling = NullValueHandling.Ignore
            }), Array.Empty<object>());
        }

        public string StartEncode(HttpListenerRequest request)
        {
            if (this.handbrakeInstance == null)
                this.handbrakeInstance = new HandBrakeInstance();
            string requestPostData = ApiRouter.GetRequestPostData(request);
            this.handbrakeInstance.Initialize(1);
            this.handbrakeInstance.StartEncode(requestPostData);
            return (string)null;
        }

        public string StopEncode(HttpListenerRequest request)
        {
            if (this.handbrakeInstance != null)
                this.handbrakeInstance.StopEncode();
            return (string)null;
        }

        public string PauseEncode(HttpListenerRequest request)
        {
            if (this.handbrakeInstance != null)
                this.handbrakeInstance.PauseEncode();
            return (string)null;
        }

        public string ResumeEncode(HttpListenerRequest request)
        {
            if (this.handbrakeInstance != null)
                this.handbrakeInstance.ResumeEncode();
            return (string)null;
        }

        public string PollEncodeProgress(HttpListenerRequest request)
        {
            if (this.handbrakeInstance == null)
                ;
            return (string)null;
        }

        public string SetConfiguration(HttpListenerRequest request)
        {
            return (string)null;
        }

        private static string GetRequestPostData(HttpListenerRequest request)
        {
            if (!request.HasEntityBody)
                return (string)null;
            using (Stream inputStream = request.InputStream)
            {
                using (StreamReader streamReader = new StreamReader(inputStream, request.ContentEncoding))
                    return streamReader.ReadToEnd();
            }
        }
    }
}
