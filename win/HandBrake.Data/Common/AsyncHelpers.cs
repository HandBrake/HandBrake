// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AsyncHelpers.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Common
{
    using System;
    using System.Threading.Tasks;

    /// <summary>
    /// Helpers for Async programming.
    /// </summary>
    public static class AsyncHelpers
    {
        /// <summary>
        /// Helpers to get an Async result synchronously without freezing the thread.
        /// </summary>
        /// <typeparam name="T">Type of Result.</typeparam>
        /// <param name="func">Task to call</param>
        /// <returns>Sync Result.</returns>
        public static T GetThreadedResult<T>(Func<Task<T>> func)
        {
            TaskCompletionSource<T> waiter = new TaskCompletionSource<T>();
            Task.Run(async () =>
            {
                T result = default(T);

                var task = func();
                if (task != null)
                {
                    result = await task;
                }

                waiter.TrySetResult(result);
            });

            return waiter.Task.Result;
        }
    }
}