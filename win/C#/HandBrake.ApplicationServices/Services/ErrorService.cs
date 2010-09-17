/*  ErrorService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.IO;
    using System.Threading;

    using Interfaces;
    using Views;

    /// <summary>
    /// The Error Service
    /// </summary>
    public class ErrorService : IErrorService
    {
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
            try
            {
                Thread newThread = new Thread(new ParameterizedThreadStart(WriteExceptionToFile));
                newThread.Start(shortError + Environment.NewLine + longError);
            }
            catch (Exception)
            {
                // Do Nothing
            }

            ExceptionWindow window = new ExceptionWindow();
            window.Setup(shortError, longError);
            window.Show();
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
        public void WriteExceptionToFile(object state)
        {
            try
            {
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string file = Path.Combine(logDir, string.Format("Exception_{0}.txt", DateTime.Now.Ticks));

                if (!File.Exists(file))
                {
                    using (StreamWriter streamWriter = new StreamWriter(file))
                    {
                        streamWriter.WriteLine(state.ToString());
                        streamWriter.Close();
                        streamWriter.Dispose();
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
