// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPortService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IPortService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    public interface IPortService
    {
        int GetOpenPort(int startPort);

        void FreePort(int port);
    }
}
