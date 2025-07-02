// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HttpRequestBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HttpRequestBase type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Net;
    using System.Net.Http;
    using System.Printing;
    using System.Text;
    using System.Threading.Tasks;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Instance.Model;

    public class HttpRequestBase
    {
        protected string serverUrl;

        protected int port;

        protected string base64Token;

        public async Task<ServerResponse> MakeHttpJsonPostRequest(string urlPath, string json)
        {
            if (string.IsNullOrEmpty(json))
            {
                throw new InvalidOperationException("No Post Values Found.");
            }

            HttpClient client = BuildHttpClient();

            using (client)
            {
                HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, this.serverUrl + urlPath);
                if (!string.IsNullOrEmpty(this.base64Token))
                {
                    requestMessage.Headers.Add("token", this.base64Token);
                }

                requestMessage.Content = new StringContent(json, Encoding.UTF8, "application/json");

                using (HttpResponseMessage response = await client.SendAsync(requestMessage))
                {
                    if (response != null)
                    {
                        string returnContent = await response.Content.ReadAsStringAsync();
                        ServerResponse serverResponse = new ServerResponse(response.IsSuccessStatusCode, returnContent, response.StatusCode.ToString());

                        return serverResponse;
                    }
                }
            }

            return null;
        }

        public async Task<ServerResponse> MakeHttpGetRequest(string urlPath)
        {
            HttpClient client = BuildHttpClient();

            using (client)
            {
                HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, this.serverUrl + urlPath);
                if (!string.IsNullOrEmpty(this.base64Token))
                {
                    requestMessage.Headers.Add("token", this.base64Token);
                }

                using (HttpResponseMessage response = await client.SendAsync(requestMessage))
                {
                    if (response != null)
                    {
                        string returnContent = await response.Content.ReadAsStringAsync();
                        ServerResponse serverResponse = response.StatusCode == HttpStatusCode.Unauthorized 
                            ? new ServerResponse(false, returnContent, response.StatusCode.ToString()) 
                            : new ServerResponse(response.IsSuccessStatusCode, returnContent, response.StatusCode.ToString());

                        return serverResponse;
                    }
                }
            }

            return null;
        }

        private HttpClient BuildHttpClient()
        {
            HttpClient client;
            if (Portable.IsSystemProxyDisabled())
            {
                var handler = new HttpClientHandler
                              {
                                  UseProxy = false // Ignore system proxy settings
                              };

                client = new HttpClient(handler) { Timeout = TimeSpan.FromSeconds(20) };
            }
            else
            {
                client = new HttpClient() { Timeout = TimeSpan.FromSeconds(20) };
            }

            return client;
        }
    }
}
