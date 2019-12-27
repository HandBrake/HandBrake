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
    using System;
    using System.IO;
    using System.Net;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;
    using HandBrake.Worker.Logging;
    using HandBrake.Worker.Logging.Interfaces;
    using HandBrake.Worker.Logging.Models;

    using Newtonsoft.Json;

    public class ApiRouter
    {
        private readonly JsonSerializerSettings jsonNetSettings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
        private HandBrakeInstance handbrakeInstance;
        private ILogHandler logHandler;

        public string Initialise(int verbosity)
        {
            if (this.handbrakeInstance == null)
            {
                this.handbrakeInstance = new HandBrakeInstance();
            }

            if (this.logHandler == null)
            {
                this.logHandler = new LogHandler();
            }

            this.handbrakeInstance.Initialize(verbosity, true);

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

            Console.WriteLine(requestPostData);
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


        /* Logging API */

        // POST - JSON
        public string ConfigureLogging(HttpListenerRequest request)
        {
            string requestPostData = GetRequestPostData(request);

            JsonSerializerSettings settings = new JsonSerializerSettings
                                              {
                                                  NullValueHandling = NullValueHandling.Ignore,
                                              };
            LogHandlerConfig config;
            if (!string.IsNullOrEmpty(requestPostData))
            {
                config = JsonConvert.DeserializeObject<LogHandlerConfig>(requestPostData, settings);
            }
            else
            {
                config = new LogHandlerConfig(false, null, false, "Logging Not Configured!");
            }

            this.logHandler.ConfigureLogging(config);
            return null;
        }
        
        // GET
        public string GetFullLog(HttpListenerRequest request)
        {
            return this.logHandler.GetFullLog();
        }

        // POST
        public string GetLogFromIndex(HttpListenerRequest request)
        {
            string requestPostData = GetRequestPostData(request);

            if (int.TryParse(requestPostData, out int index))
            {
                return this.logHandler.GetLogFromIndex(index);
            }

            return null;
        }

        // POST
        public string GetLatestLogIndex(HttpListenerRequest request)
        {
            return this.logHandler.GetLatestLogIndex().ToString();
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
