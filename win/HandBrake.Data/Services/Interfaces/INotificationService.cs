// --------------------------------------------------------------------------------------------------------------------
// <copyright file="INotificationService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services.Interfaces
{
    /// <summary>
    /// Functions related to notifying the User.
    /// </summary>
    public interface INotificationService
    {
        /// <summary>
        /// Creates a notification to the user with the provided text.
        /// </summary>
        /// <param name="text">Text</param>
        void Notify(string text);
    }
}