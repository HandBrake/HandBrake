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
    using System.Net.Http;
    using System.Text;
    using System.Threading.Tasks;

    using HandBrakeWPF.Instance.Model;

    using Newtonsoft.Json;

    public class HttpRequestBase
    {
        protected readonly JsonSerializerSettings jsonNetSettings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
        protected HttpClient client = new HttpClient();
        protected string serverUrl;
        protected int port;

        public async Task<ServerResponse> MakeHttpJsonPostRequest(string urlPath, string json)
        {
            if (string.IsNullOrEmpty(json))
            {
                throw new InvalidOperationException("No Post Values Found.");
            }

            StringContent content = new StringContent(json, Encoding.UTF8, "application/json");
            HttpResponseMessage response = await client.PostAsync(this.serverUrl + urlPath, content);
            if (response != null)
            {
                string returnContent = await response.Content.ReadAsStringAsync();
                ServerResponse serverResponse = new ServerResponse(response.IsSuccessStatusCode, returnContent);

                return serverResponse;
            }

            return null;
        }

        public async Task<ServerResponse> MakeHttpGetRequest(string urlPath)
        {
            HttpResponseMessage response = await client.GetAsync(this.serverUrl + urlPath);
            if (response != null)
            {
                string returnContent = await response.Content.ReadAsStringAsync();
                ServerResponse serverResponse = new ServerResponse(response.IsSuccessStatusCode, returnContent);

                return serverResponse;
            }

            return null;
        }
    }
}
