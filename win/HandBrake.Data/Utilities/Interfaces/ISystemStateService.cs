// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISystemStateService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Functions related to the System State.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities.Interfaces
{
    /// <summary>
    /// Functions related to the System State.
    /// </summary>
    public interface ISystemStateService
    {
        /// <summary>
        /// Gets a value indicating whether Power State Changes is Supported.
        /// </summary>
        bool SupportsPowerStateChange { get; }

        /// <summary>
        /// Gets a value indicating whether Hibernate is supported.
        /// </summary>
        bool SupportsHibernate { get; }

        /// <summary>
        /// Gets a value indicating whether Log Off is supported.
        /// </summary>
        bool SupportsLogOff { get; }

        /// <summary>
        /// Gets a value indicating whether Lock is supported;
        /// </summary>
        bool SupportsLock { get; }

        /// <summary>
        /// Prevents the computer or application from sleeping or suspending.
        /// </summary>
        void PreventSleep();

        /// <summary>
        /// Allows the computer and application to sleep and suspend.
        /// </summary>
        void AllowSleep();

        /// <summary>
        /// Shuts the computer down.
        /// </summary>
        void Shutdown();

        /// <summary>
        /// Logs off the computer.
        /// </summary>
        void LogOff();

        /// <summary>
        /// Suspends the computer.
        /// </summary>
        void Suspend();

        /// <summary>
        /// Hibernates the computer.
        /// </summary>
        void Hibernate();

        /// <summary>
        /// Locks the computer.
        /// </summary>
        void Lock();

        /// <summary>
        /// Quits the Program.
        /// </summary>
        void Quit();
    }
}