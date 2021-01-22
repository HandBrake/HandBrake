// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HttpServer.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   This is a service worker for the HandBrake app. It allows us to run encodes / scans in a separate process easily.
//   All API's expose the ApplicationServices models as JSON.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Net;
    using System.Text;
    using System.Threading;

    using HandBrake.Worker.Services.Interfaces;

    public class HttpServer
    {
        private readonly ITokenService tokenService;

        private readonly HttpListener httpListener = new HttpListener();
        private readonly Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers;

        private readonly bool failedStart;

        public HttpServer(Dictionary<string, Func<HttpListenerRequest, string>> apiCalls, int port, ITokenService tokenService)
        {
            if (!HttpListener.IsSupported)
            {
                throw new NotSupportedException("HttpListener not supported on this computer.");
            }

            this.tokenService = tokenService;

            // Store the Handlers
            this.apiHandlers = new Dictionary<string, Func<HttpListenerRequest, string>>(apiCalls);

            if (!tokenService.IsTokenSet())
            {
                Console.WriteLine("Worker: No Token Set.");

                Console.WriteLine();
                Console.WriteLine("API Information: ");
                Console.WriteLine("All calls require a 'token' in the HTTP header ");
            }

            foreach (KeyValuePair<string, Func<HttpListenerRequest, string>> api in apiCalls)
            {
                string url = string.Format("http://127.0.0.1:{0}/{1}/", port, api.Key);
                this.httpListener.Prefixes.Add(url);

                if (!tokenService.IsTokenSet())
                {
                    Console.WriteLine(url);
                }
            }

            if (!tokenService.IsTokenSet())
            {
                Console.WriteLine();
            }

            try
            {
                this.httpListener.Start();
            }
            catch (Exception e)
            {
                this.failedStart = true;

                Console.WriteLine("Worker: Unable to start HTTP Server. Maybe the port {0} is in use?", port);
                Console.WriteLine("Worker Exception: " + e);
            }
        }

        public bool Run()
        {
            if (this.failedStart)
            {
                return false;
            }

            ThreadPool.QueueUserWorkItem(o =>
            {
                try
                {
                    while (this.httpListener.IsListening)
                    {
                        ThreadPool.QueueUserWorkItem(
                            (c) =>
                                {
                                    var context = c as HttpListenerContext;
                                    if (context == null)
                                    {
                                        return;
                                    }

                                    try
                                    {
                                        string path = context.Request.RawUrl.TrimStart('/').TrimEnd('/');
                                        string token = context.Request.Headers.Get("token");

                                        if (!path.Equals("Version") && !tokenService.IsAuthenticated(token))
                                        {
                                            string rstr = "Worker: Access Denied. The token provided in the HTTP header was not valid.";
                                            Console.WriteLine(rstr);
                                            byte[] buf = Encoding.UTF8.GetBytes(rstr);
                                            context.Response.StatusCode = (int)HttpStatusCode.Unauthorized;
                                            context.Response.ContentLength64 = buf.Length;
                                            context.Response.OutputStream.Write(buf, 0, buf.Length);
                                            return;
                                        }

                                        Debug.WriteLine("Handling call to: " + path);

                                        if (this.apiHandlers.TryGetValue(path, out var actionToPerform))
                                        {
                                            string rstr = actionToPerform(context.Request);
                                            if (!string.IsNullOrEmpty(rstr))
                                            {
                                                byte[] buf = Encoding.UTF8.GetBytes(rstr);
                                                context.Response.ContentLength64 = buf.Length;
                                                context.Response.OutputStream.Write(buf, 0, buf.Length);
                                            }
                                        }
                                        else
                                        {
                                            string rstr = "Error, There is a missing API handler.";
                                            byte[] buf = Encoding.UTF8.GetBytes(rstr);
                                            context.Response.ContentLength64 = buf.Length;
                                            context.Response.OutputStream.Write(buf, 0, buf.Length);
                                        }
                                    }
                                    catch (Exception exc)
                                    {
                                        Console.WriteLine("Worker: Listener Thread: " + exc);
                                    }
                                    finally
                                    {
                                        context?.Response.OutputStream.Close();
                                    }
                                },
                            this.httpListener.GetContext());
                    }
                }
                catch (Exception exc)
                {
                    Console.WriteLine("Worker: " + exc);
                }
            });

            return true;
        }

        public void Stop()
        {
            this.httpListener.Stop();
            this.httpListener.Close();
        }
    }
}