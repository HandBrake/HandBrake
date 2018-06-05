// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TaskState.cs" company="HandBrake Project (http://handbrake.fr)">
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
        public static TaskState Idle = new TaskState("IDLE");
        public static TaskState Scanning = new TaskState("SCANNING");
        public static TaskState ScanDone = new TaskState("SCANDONE");
        public static TaskState Working = new TaskState("WORKING");
        public static TaskState Paused = new TaskState("PAUSED");
        public static TaskState Searching = new TaskState("SEARCHING");
        public static TaskState WorkDone = new TaskState("WORKDONE");
        public static TaskState Muxing = new TaskState("MUXING");
        public static TaskState Unknown = new TaskState("UNKNOWN");

        private static readonly Dictionary<string, TaskState> taskStates = new Dictionary<string, TaskState>();

        static TaskState()
        {
            taskStates.Add("IDLE", Idle);
            taskStates.Add("SCANNING", Scanning);
            taskStates.Add("SCANDONE", ScanDone);
            taskStates.Add("WORKING", Working);
            taskStates.Add("PAUSED", Paused);
            taskStates.Add("SEARCHING", Searching);
            taskStates.Add("WORKDONE", WorkDone);
            taskStates.Add("MUXING", Muxing);
            taskStates.Add("UNKNOWN", Unknown);
        }

        public TaskState(string code)
        {
            this.Code = code;
        }

        public string Code { get; }

        public static TaskState FromRepositoryValue(string code)
        {
            TaskState state = null;
            if (taskStates.TryGetValue(code, out state))
            {
                return state;
            }

            return null;
        }
    }
}
