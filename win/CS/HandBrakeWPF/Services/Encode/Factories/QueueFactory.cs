// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the QueueFactory type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Factories
{
    using System.Collections.Generic;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Json.Queue;
    using HandBrake.Interop.Interop.Providers.Interfaces;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;

    using Newtonsoft.Json;

    /// <summary>
    /// The queue factory.
    /// </summary>
    public class QueueFactory
    {
        /// <summary>
        /// For a givent set of tasks, return the Queue JSON that can be used for the CLI.
        /// </summary>
        /// <param name="tasks">
        /// The tasks.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetQueueJson(List<EncodeTask> tasks, HBConfiguration configuration)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
            {
                NullValueHandling = NullValueHandling.Ignore,
            };

            IHbFunctionsProvider provider = IoC.Get<IHbFunctionsProvider>(); // TODO remove IoC call.
            IHbFunctions hbFunctions = provider.GetHbFunctionsWrapper();

            List<Task> queueJobs = new List<Task>();
            foreach (var item in tasks)
            {
                Task task = new Task { Job = EncodeTaskFactory.Create(item, configuration, hbFunctions) };
                queueJobs.Add(task);
            }

            return JsonConvert.SerializeObject(queueJobs, Formatting.Indented, settings);
        }

        public static List<Task> GetQueue(string content)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
            List<Task> tasks = JsonConvert.DeserializeObject<List<Task>>(content, settings);
            return tasks;
        }
    }
}
