// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IEncodeInstance.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the encode portions of the IHandBrakeInstance
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces
{
    using System;

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;

    public interface IEncodeInstance
    {
        /// <summary>
        /// Fires when an encode has completed.
        /// </summary>
        event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        /// <summary>
        /// Fires for progress updates when encoding.
        /// </summary>
        event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity">
        /// The code for the logging verbosity to use.
        /// </param>
        void Initialize(int verbosity, bool noHardware);

        /// <summary>
        /// Frees any resources associated with this object.
        /// </summary>
        void Dispose();

        /// <summary>
        /// Pauses the current encode.
        /// </summary>
        void PauseEncode();

        /// <summary>
        /// Resumes a paused encode.
        /// </summary>
        void ResumeEncode();

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="jobToStart">
        /// The job to start.
        /// </param>
        void StartEncode(JsonEncodeObject jobToStart);

        /// <summary>
        /// Stops the current encode.
        /// </summary>
        void StopEncode();

        /// <summary>
        /// Get the current Encode State.
        /// </summary>
        /// <returns>A JsonState object</returns>
        JsonState GetEncodeProgress();
    }
}