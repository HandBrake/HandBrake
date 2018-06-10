// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Program.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Manage the HandBrake Worker Process Service.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker
{
    using System;
    using System.Collections.Generic;
    using System.Net;

    public class Program
    {
        /*
         * TODO
         * Methods:
         *   1. Fetch Log
         *   2. Fetch Log since last index.
         * Services:
         *   3. Support for connecting via sockets.
         *   4. All methods will return a json state object response.
         */

        private static ApiRouter router;

        public static void Main(string[] args)
        {
            Console.WriteLine("Starting Web Server ...");
            router = new ApiRouter();

            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers = RegisterApiHandlers();

            int port = 8080; // Default Port;

            if (args.Length != 0)
            {
                foreach (string argument in args)
                {
                    if (argument.StartsWith("--port"))
                    {
                        string portStr = argument.TrimStart("--port=".ToCharArray());
                        if (int.TryParse(portStr, out var parsedPort))
                        {
                            port = parsedPort;
                        }
                    }
                }
            }

            Console.WriteLine("Using Port: {0}", port);

            HttpServer webServer = new HttpServer(apiHandlers, port);
            webServer.Run();

            Console.WriteLine("Webserver Started");
            Console.WriteLine("Press any key to exit");

            Console.ReadKey(); // Block from closing.

            webServer.Stop();
        }

        public static Dictionary<string, Func<HttpListenerRequest, string>> RegisterApiHandlers()
        {
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers =
                new Dictionary<string, Func<HttpListenerRequest, string>>();

            apiHandlers.Add("Version", router.GetVersionInfo); 
            apiHandlers.Add("StartEncode", router.StartEncode);
            apiHandlers.Add("PauseEncode", router.PauseEncode);
            apiHandlers.Add("ResumeEncode", router.ResumeEncode);
            apiHandlers.Add("StopEncode", router.StopEncode);
            apiHandlers.Add("PollEncodeProgress", router.PollEncodeProgress);
            apiHandlers.Add("SetConfiguration", router.SetConfiguration);
            apiHandlers.Add("Initialise", router.Initialise);
           
            return apiHandlers;
        }
    }
}

