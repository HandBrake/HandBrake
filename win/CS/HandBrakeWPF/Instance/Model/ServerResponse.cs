// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ServerResponse.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A model of the response from the HandBrake Worker instance.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance.Model
{
    public class ServerResponse
    {
        public ServerResponse(bool wasSuccessful, string jsonResponse, string statusCode)
        {
            this.WasSuccessful = wasSuccessful;
            this.JsonResponse = jsonResponse;
            this.StatusCode = statusCode;
        }

        public bool WasSuccessful { get; private set; }

        public string JsonResponse { get; private set; }

        public string StatusCode { get; private set; }
    }
}
