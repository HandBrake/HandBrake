/*  Program.cs
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;

    using Handbrake.Properties;

    using Presets;

    /// <summary>
    /// HandBrake Starts Here
    /// </summary>
    public static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        public static void Main(string[] args)
        {
            InstanceId = Process.GetProcessesByName("HandBrake").Length;

            // Handle any unhandled exceptions
            AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);

            // Attempt to upgrade / keep the users settings between versions
            if (Settings.Default.UpdateRequired)
            {
                Settings.Default.Upgrade();
                Settings.Default.UpdateRequired = false;
            }

            // Make sure we have any pre-requesits before trying to launch
            const string FailedInstall = "HandBrake is not installed properly. Please reinstall HandBrake. \n\n";
            const string NightlyCLIMissing =
                "If you have downloaded the \"HandBrakeGUI\" nightly, " +
                "please make sure you have also downloaded the \"HandBrakeCLI\" nightly and extracted it's contents to the same folder. ";
            string missingFiles = string.Empty;

            // Verify HandBrakeCLI.exe exists
            if (!File.Exists(Path.Combine(Application.StartupPath, "HandBrakeCLI.exe")))
            {
                missingFiles += "\"HandBrakeCLI.exe\" was not found.";
            }

            if (missingFiles != string.Empty)
            {
                MessageBox.Show(
                    FailedInstall + missingFiles + "\n\n" + NightlyCLIMissing,
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            // Check were not running on a screen that's going to cause some funnies to happen.
            Screen scr = Screen.PrimaryScreen;
            if ((scr.Bounds.Width < 1024) || (scr.Bounds.Height < 620))
                MessageBox.Show("Your system does not meet the minimum requirements for HandBrake. \n" + "Your screen is running at: " + scr.Bounds.Width + "x" + scr.Bounds.Height + " \nScreen resolution is too Low. Must be 1024x620 or greater", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
            {
                string logDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\logs");
                if (!Directory.Exists(logDir))
                    Directory.CreateDirectory(logDir);

                if (!File.Exists(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\presets.xml")))
                {
                    PresetsHandler x = new PresetsHandler();
                    x.UpdateBuiltInPresets();
                }

                InitializeApplicationServices();

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new frmMain(args));
            }
        }

        /// <summary>
        /// Initialize App Services
        /// </summary>
        private static void InitializeApplicationServices()
        {
            string versionId = String.Format("Windows GUI {1} {0}", Settings.Default.hb_build, Settings.Default.hb_version);
            Init.SetupSettings(versionId, InstanceId, Settings.Default.CompletionOption, Settings.Default.noDvdNav,
                               Settings.Default.growlEncode, Settings.Default.growlQueue,
                               Settings.Default.processPriority, Settings.Default.saveLogPath, Settings.Default.saveLogToSpecifiedPath,
                               Settings.Default.saveLogWithVideo, Settings.Default.showCliForInGuiEncodeStatus, Settings.Default.preventSleep);
        }

        /// <summary>
        /// Throw up an error message for any unhandled exceptions.
        /// </summary>
        /// <param name="sender">The sender</param>
        /// <param name="e">Unhandled Exception EventArgs </param>
        private static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            try
            {
                frmExceptionWindow exceptionWindow = new frmExceptionWindow();
                exceptionWindow.Setup("An Unknown Error has occured.", e.ExceptionObject.ToString());
                exceptionWindow.ShowDialog();
            }
            catch (Exception)
            {
                MessageBox.Show(
                    "An Unknown Error has occured. \n\n Exception:" + e.ExceptionObject,
                    "Unhandled Exception",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }


        public static int InstanceId = 0;


    }
}