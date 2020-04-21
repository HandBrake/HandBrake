// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ActiveJobCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Queue.JobEventArgs
{
    using System;

    using HandBrakeWPF.Services.Encode.EventArgs;

    public class ActiveJobCompletedEventArgs : EventArgs
    {
        public ActiveJobCompletedEventArgs(ActiveJob job, EncodeCompletedEventArgs encodeCompletedEventArgs)
        {
            this.EncodeEventArgs = encodeCompletedEventArgs;
            this.Job = job;
        }

        public ActiveJob Job { get; private set; }

        public EncodeCompletedEventArgs EncodeEventArgs { get; private set; }
    }
}
