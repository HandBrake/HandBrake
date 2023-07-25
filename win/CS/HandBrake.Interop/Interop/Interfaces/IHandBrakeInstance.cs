// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IHandBrakeInstance.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Interface for HandBrakeInstance
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces
{
    using HandBrake.Interop.Interop.Json.State;

    /// <summary>
    /// The Interface for HandBrakeInstance
    /// </summary>
    public interface IHandBrakeInstance
    {
        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        string Version { get; }

        /// <summary>
        /// Gets the HandBrake build number.
        /// </summary>
        int Build { get; }


        bool IsRemoteInstance { get; }

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity">
        /// The code for the logging verbosity to use.
        /// </param>
        /// <param name="noHardware">
        /// Turn off Hardware Acceleration 
        /// </param>
        void Initialize(int verbosity, bool noHardware);

        /// <summary>
        /// Get the current Progress State.
        /// </summary>
        /// <returns>A JsonState object</returns>
        JsonState GetProgress();
    }
}