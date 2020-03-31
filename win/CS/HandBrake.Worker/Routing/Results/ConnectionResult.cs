// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ConnectionResult.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ConnectionResult type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing.Results
{
    public class ConnectionResult
    {
        public ConnectionResult()
        {
        }

        public ConnectionResult(bool isSuccessfulConnection, string token, string error)
        {
            this.IsSuccessfulConnection = isSuccessfulConnection;
            this.Token = token;
            this.Error = error;
        }

        public bool IsSuccessfulConnection { get; set; }
        
        public string Token { get; set; }

        public string Error { get; set; }
    }
}
