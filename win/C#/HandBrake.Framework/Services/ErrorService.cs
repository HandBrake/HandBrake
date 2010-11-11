/*  ErrorService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Services
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.Framework.Services.Interfaces;
    using HandBrake.Framework.Views;

    /// <summary>
    /// The Error Service
    /// </summary>
    public class ErrorService : IErrorService
    {
        private int exceptionCount;

        /// <summary>
        /// Show an Error Window
        /// </summary>
        /// <param name="shortError">
        /// The short error message for the user to read
        /// </param>
        /// <param name="longError">
        /// Exception string or advanced details
        /// </param>
        public void ShowError(string shortError, string longError)
        {
            exceptionCount++;

            try
            {
                Thread newThread = new Thread(new ParameterizedThreadStart(this.LogError));
                newThread.Start(shortError + Environment.NewLine + longError);
            }
            catch (Exception)
            {
                // Do Nothing
            }

            if (exceptionCount > 30)
            {
                // If we are getting a large number of exceptions, just die out. We don't want to fill the users drive with a ton 
                // of exception files.
                return;
            }

            ExceptionWindow window = new ExceptionWindow();
            window.Setup(shortError, longError);

            // This seems far from ideal so maybe have a think about a better way of doing this.
            // This method can be called from UI and worker threads, so the ExcWindow needs to be called on the UI thread or on it's on UI thread.
            Application.Run(window); 
        }

        /// <summary>
        /// Show a Notice or Warning Message.
        /// </summary>
        /// <param name="notice">
        /// The text to display to the user
        /// </param>
        /// <param name="isWarning">
        /// Is a warning window, show the warning icon instead of the notice
        /// </param>
        public void ShowNotice(string notice, bool isWarning)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Write Exceptions out to log files
        /// </summary>
        /// <param name="state">
        /// The state.
        /// </param>
        public void LogError(object state)
        {
            try
            {
                if (exceptionCount > 30)
                {
                    // If we are getting a large number of exceptions, just die out. We don't want to fill the users drive with a ton 
                    // of exception files.
                    return;
                }

                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string file = Path.Combine(logDir, string.Format("Exception_{0}.txt", DateTime.Now.Ticks));

                if (!File.Exists(file))
                {
                    using (StreamWriter streamWriter = new StreamWriter(file))
                    {
                        streamWriter.WriteLine(state.ToString());
                    }
                }
            }
            catch
            {
                return; // Game over. Stop digging.
            }
        }
    }
}
