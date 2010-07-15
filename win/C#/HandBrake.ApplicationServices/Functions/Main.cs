/*  Main.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System.Diagnostics;

    /// <summary>
    /// Useful functions which various screens can use.
    /// </summary>
    public static class Main
    {
        /// <summary>
        /// Get the Process ID of HandBrakeCLI for the current instance.
        /// </summary>
        /// <returns>A list of processes</returns>
        public static Process[] GetCliProcess()
        {
            return Process.GetProcessesByName("HandBrakeCLI");
        }

        /// <summary>
        /// Show the Exception Window
        /// </summary>
        /// <param name="shortError">
        /// The short error.
        /// </param>
        /// <param name="longError">
        /// The long error.
        /// </param>
        public static void ShowExceptiowWindow(string shortError, string longError)
        {
            frmExceptionWindow exceptionWindow = new frmExceptionWindow();
            exceptionWindow.Setup(shortError, longError);
            exceptionWindow.Show();
        }
    }
}