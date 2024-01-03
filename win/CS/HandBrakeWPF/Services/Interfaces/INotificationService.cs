// --------------------------------------------------------------------------------------------------------------------
// <copyright file="INotificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    public interface INotificationService
    {
        bool SendNotification(string header, string content);
        void Uninstall();
        void Shutdown();
    }
}
