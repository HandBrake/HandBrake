// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TaskState.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The status of the current task being processed.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.State
{
    using System.Collections.Generic;

    public class TaskState
    {
        private static readonly Dictionary<string, TaskState> TaskStates = new Dictionary<string, TaskState>();

        public static TaskState Idle = new TaskState("IDLE");
        public static TaskState Scanning = new TaskState("SCANNING");
        public static TaskState ScanDone = new TaskState("SCANDONE");
        public static TaskState Working = new TaskState("WORKING");
        public static TaskState Paused = new TaskState("PAUSED");
        public static TaskState Searching = new TaskState("SEARCHING");
        public static TaskState WorkDone = new TaskState("WORKDONE");
        public static TaskState Muxing = new TaskState("MUXING");
        public static TaskState Unknown = new TaskState("UNKNOWN");

        static TaskState()
        {
            TaskStates.Add("IDLE", Idle);
            TaskStates.Add("SCANNING", Scanning);
            TaskStates.Add("SCANDONE", ScanDone);
            TaskStates.Add("WORKING", Working);
            TaskStates.Add("PAUSED", Paused);
            TaskStates.Add("SEARCHING", Searching);
            TaskStates.Add("WORKDONE", WorkDone);
            TaskStates.Add("MUXING", Muxing);
            TaskStates.Add("UNKNOWN", Unknown);
        }

        public TaskState(string code)
        {
            this.Code = code;
        }

        public string Code { get; }

        public static TaskState FromRepositoryValue(string code)
        {
            if (string.IsNullOrEmpty(code))
            {
                return null;
            }

            if (TaskStates.TryGetValue(code, out TaskState state))
            {
                return state;
            }

            return null;
        }
    }
}
