// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ConnectionRegistrar.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ConnectionRegistrar type.
// </summary>
// <remarks>
//  To be clear, this is NOT a security service!
//  This service simply allows the UI to verify which worker process it is connected to.
// </remarks>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Registration
{
    using System;
    using System.Net;

    using HandBrake.Worker.Registration.Model;
    using HandBrake.Worker.Utilities;

    using Newtonsoft.Json;

    public class ConnectionRegistrar
    {
        private readonly JsonSerializerSettings jsonNetSettings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
        private string token;

        public ConnectionRegistrar()
        {
        }

        public string Pair(HttpListenerRequest request)
        {
            string tokenKey = HttpUtilities.GetRequestPostData(request);


            if (!string.IsNullOrEmpty(token))
            {
                return JsonConvert.SerializeObject(new ConnectionResult(false, null, "Already Paired"), Formatting.Indented, this.jsonNetSettings);
            }

            this.token = tokenKey;

            return JsonConvert.SerializeObject(new ConnectionResult(true, tokenKey, null), Formatting.Indented, this.jsonNetSettings);
        }

        public string GetToken(HttpListenerRequest request)
        { 
            return this.token;
        }
    }
}
