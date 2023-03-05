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

    using HandBrake.Worker.Logging;
    using HandBrake.Worker.Services.Interfaces;

    public class HttpServer
    {
        private ITokenService tokenService;

        private readonly HttpListener httpListener = new HttpListener();
        private Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers;

        private bool failedStart;

        public HttpServer(Dictionary<string, Func<HttpListenerRequest, string>> apiCalls, int port, ITokenService tokenService)
        {
            try
            {
                Init(apiCalls, port, tokenService);
            }
            catch (Exception e)
            {
                ConsoleOutput.WriteLine("HandBrake Worker: Failed to Initialise: " + Environment.NewLine + e, ConsoleColor.Red);
            }
        }

        public bool Run()
        {
            httpListener.BeginGetContext(new AsyncCallback(ListenerCallback), this.httpListener);
            return true;
        }

        public void ListenerCallback(IAsyncResult result)
        {
            if (this.failedStart)
            {
                return;
            }

            if (this.httpListener.IsListening)
            {
                try
                {
                    var context = this.httpListener.EndGetContext(result);
                    lock (this.httpListener)
                    {
                        this.HandleRequest(context);
                    }
                }
                catch (Exception e)
                {
                    if (e is HttpListenerException)
                    {
                        Debug.WriteLine("Worker: " + e);
                        return;
                    }
                }
            }

            this.Run();
        }

        public void Stop()
        {
            this.httpListener.Stop();
            this.httpListener.Close();
        }

        private void Init(Dictionary<string, Func<HttpListenerRequest, string>> apiCalls, int port, ITokenService tokenService)
        {
            if (!HttpListener.IsSupported)
            {
                string rstr = string.Format(
                    "Worker: Failed to Initialise: HttpListener is not supported on this computer.{0}" 
                    + "\r\nProcess Isolation can be disabled in \"Tools Menu -> Preferences -> Advanced -> Process Isolation\".{0}",
                    Environment.NewLine);

                ConsoleOutput.WriteLine(rstr, ConsoleColor.Red);
                throw new NotSupportedException("HttpListener not supported on this computer.");
            }

            this.tokenService = tokenService;

            // Store the Handlers
            this.apiHandlers = new Dictionary<string, Func<HttpListenerRequest, string>>(apiCalls);

            if (!tokenService.IsTokenSet())
            {
                ConsoleOutput.WriteLine("HandBrake Worker: Token has not been initialised. API Access is limited!", ConsoleColor.Red);
                Console.WriteLine(Environment.NewLine);
                ConsoleOutput.WriteLine("API Information: ", ConsoleColor.Cyan);
                Console.WriteLine("All calls require a 'token' in the HTTP header ");
            }

            // Base URL
            string url = string.Format("http://127.0.0.1:{0}/", port);
            this.httpListener.Prefixes.Add(url);

            // API URLS
            foreach (KeyValuePair<string, Func<HttpListenerRequest, string>> api in apiCalls)
            {
                url = string.Format("http://127.0.0.1:{0}/{1}/", port, api.Key);
                this.httpListener.Prefixes.Add(url);

                if (!tokenService.IsTokenSet())
                {
                    Console.WriteLine(" - " + url);
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

        private void HandleRequest(HttpListenerContext context)
        {
            try
            {
                if (context == null)
                {
                    return;
                }

                Stopwatch watch = Stopwatch.StartNew();

                string path = context.Request.RawUrl.TrimStart('/').TrimEnd('/');
                string token = context.Request.Headers.Get("token");

                if (path == string.Empty)
                {
                    string rstr = string.Format("HandBrake Worker {0}{0}"
                                                + "This worker runs on localhost loopback only and is not accessible to the wider network.{0}"
                                                + "\r\nThis feature allows processing of HandBrake jobs on a background process.{0}"
                                                + "\r\nThis can be enabled and disabled in \"Tools Menu -> Preferences -> Advanced -> Process Isolation\".{0}",
                        Environment.NewLine);

                    byte[] buf = Encoding.UTF8.GetBytes(rstr);
                    context.Response.StatusCode = (int)HttpStatusCode.Unauthorized;
                    context.Response.ContentLength64 = buf.Length;
                    context.Response.OutputStream.Write(buf, 0, buf.Length);

                    watch.Stop();
                    Debug.WriteLine(string.Format(" - Processed call to: '/{0}', Took {1}ms", path, watch.ElapsedMilliseconds), ConsoleColor.White, true);
                    return;
                }

                if (!path.Equals("Version") && !tokenService.IsAuthenticated(token))
                {
                    string rstr = string.Format("HandBrake Worker: Access Denied to '/{0}'. The token provided in the HTTP header was not valid.", path);
                    ConsoleOutput.WriteLine(rstr, ConsoleColor.Red, true);
                    byte[] buf = Encoding.UTF8.GetBytes(rstr);
                    context.Response.StatusCode = (int)HttpStatusCode.Unauthorized;
                    context.Response.ContentLength64 = buf.Length;
                    context.Response.OutputStream.Write(buf, 0, buf.Length);

                    watch.Stop();
                    Debug.WriteLine(string.Format(" - Processed call to: '/{0}', Took {1}ms", path, watch.ElapsedMilliseconds), ConsoleColor.White, true);
                    return;
                }

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
                    string rstr = "HandBrake Worker: The API endpoint is unknown.";
                    byte[] buf = Encoding.UTF8.GetBytes(rstr);
                    context.Response.ContentLength64 = buf.Length;
                    context.Response.OutputStream.Write(buf, 0, buf.Length);
                }

                watch.Stop();
                Debug.WriteLine(string.Format(" - Processed call to: '/{0}', Took {1}ms", path, watch.ElapsedMilliseconds), ConsoleColor.White, true);
            }
            catch (Exception exc)
            {
                Debug.WriteLine("Worker: Listener Thread: " + exc);
            }
            finally
            {
                context?.Response.OutputStream.Close();
            }
        }
    }
}