// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TokenService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   This acts as a pairing system between the UI and the Server to avoid requests hitting the API that are not meant for it.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Services
{
    using HandBrake.Worker.Services.Interfaces;

    public class TokenService : ITokenService
    {
        private readonly object lockObject = new object();
        private string uiToken;
        
        public bool RegisterToken(string requestToken)
        {
            lock (this.lockObject)
            {
                // Don't allow users to override the current token
                if (string.IsNullOrEmpty(this.uiToken))
                {
                    this.uiToken = requestToken;
                    return true;
                }

                return false;
            }
        }

        public bool IsTokenSet()
        {
            lock (this.lockObject)
            {
                return !string.IsNullOrEmpty(this.uiToken);
            }
        }

        public bool IsAuthenticated(string token)
        {
            lock (this.lockObject)
            {
                if (string.IsNullOrEmpty(token) || string.IsNullOrEmpty(this.uiToken))
                {
                    return false;
                }

                if (token != this.uiToken)
                {
                    return false;
                }

                return true;
            }
        }
    }
}
