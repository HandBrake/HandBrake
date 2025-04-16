// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ITokenService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ITokenService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Services.Interfaces
{
    public interface ITokenService
    {
        bool RegisterToken(string request);

        bool IsTokenSet();

        bool IsAuthenticated(string token);
    }
}
