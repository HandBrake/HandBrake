// --------------------------------------------------------------------------------------------------------------------
// <copyright company="HandBrake Project (http://handbrake.fr)" file="Execute.cs">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Enables easy marshalling of code to the UI thread.
//   Borrowed from Caliburn Micro.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using System.Windows;
    using System.Windows.Threading;

    /// <summary>
    /// Enables easy marshalling of code to the UI thread.
    /// </summary>
    public static class Execute
    {
        private static System.Action<System.Action> executor = (System.Action<System.Action>)(action => action());
        private static Dispatcher dispatcher;
        private static bool? inDesignMode;

        /// <summary>
        /// Indicates whether or not the framework is in design-time mode.
        /// </summary>
        public static bool InDesignMode
        {
            get
            {
                if (!Execute.inDesignMode.HasValue)
                {
                    Execute.inDesignMode = new bool?((bool)DependencyPropertyDescriptor.FromProperty(DesignerProperties.IsInDesignModeProperty, typeof(FrameworkElement)).Metadata.DefaultValue);
                    if (!Execute.inDesignMode.GetValueOrDefault(false) && Process.GetCurrentProcess().ProcessName.StartsWith("devenv", StringComparison.Ordinal))
                        Execute.inDesignMode = new bool?(true);
                }
                return Execute.inDesignMode.GetValueOrDefault(false);
            }
        }

        /// <summary>
        /// Initializes the framework using the current dispatcher.
        /// </summary>
        public static void InitializeWithDispatcher()
        {
            Execute.dispatcher = Dispatcher.CurrentDispatcher;
            Execute.executor = (System.Action<System.Action>)null;
        }

        /// <summary>
        /// Resets the executor to use a non-dispatcher-based action executor.
        /// </summary>
        public static void ResetWithoutDispatcher()
        {
            executor = (System.Action<System.Action>)(action => action());
            dispatcher = (Dispatcher)null;
        }

        /// <summary>
        /// Sets a custom UI thread marshaller.
        /// </summary>
        /// <param name="marshaller">The marshaller.</param>
        [Obsolete]
        public static void SetUIThreadMarshaller(System.Action<System.Action> marshaller)
        {
            Execute.executor = marshaller;
            Execute.dispatcher = (Dispatcher)null;
        }

        private static void ValidateDispatcher()
        {
            if (Execute.dispatcher == null)
                throw new InvalidOperationException("Not initialized with dispatcher.");
        }

        /// <summary>
        /// Executes the action on the UI thread asynchronously.
        /// </summary>
        /// <param name="action">The action to execute.</param>
        public static void BeginOnUIThread(this System.Action action)
        {
            Execute.ValidateDispatcher();
            Execute.dispatcher.BeginInvoke((Delegate)action);
        }

        /// <summary>
        /// Executes the action on the UI thread asynchronously.
        /// </summary>
        /// <param name="action">The action to execute.</param>
        public static Task OnUIThreadAsync(this System.Action action)
        {
            Execute.ValidateDispatcher();
            TaskCompletionSource<object> taskSource = new TaskCompletionSource<object>();
            System.Action action1 = (System.Action)(() =>
            {
                try
                {
                    action();
                    taskSource.SetResult((object)null);
                }
                catch (Exception ex)
                {
                    taskSource.SetException(ex);
                }
            });
            Execute.dispatcher.BeginInvoke((Delegate)action1);
            return (Task)taskSource.Task;
        }

        private static bool CheckAccess()
        {
            if (Execute.dispatcher != null)
                return Execute.dispatcher.CheckAccess();
            return true;
        }

        /// <summary>
        /// Executes the action on the UI thread.
        /// </summary>
        /// <param name="action">The action to execute.</param>
        public static void OnUIThread(this System.Action action)
        {
            if (Execute.executor != null)
                Execute.executor(action);
            else if (Execute.CheckAccess())
                action();
            else
                Execute.OnUIThreadAsync(action).Wait();
        }
    }
}
