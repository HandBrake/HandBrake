/*  Program.cs
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.IO;
    using System.Windows.Forms;
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
        public static void Main()
        {
            const string failedInstall = "HandBrake is not installed properly. Please reinstall HandBrake. \n\n";
            const string nightlyCLIMissing =
                "If you have downloaded the \"HandBrakeGUI\" nightly, " +
                "please make sure you have also downloaded the \"HandBrakeCLI\" nightly and extracted it's contents to the same folder. ";
            string missingFiles = string.Empty;

            // Verify HandBrakeCLI.exe and ilibgcc_s_sjlj-1.dll exists
            if (!File.Exists(Path.Combine(Application.StartupPath, "HandBrakeCLI.exe")))
            {
                missingFiles += "\"HandBrakeCLI.exe\" was not found.";
            }

            if (!File.Exists(Path.Combine(Application.StartupPath, "libgcc_s_sjlj-1.dll")))
            {
                missingFiles += "\n\"libgcc_s_sjlj-1.dll\" was not found.";
            }

            if (missingFiles != string.Empty)
            {
                MessageBox.Show(failedInstall + missingFiles + "\n\n"+ nightlyCLIMissing, "Error", MessageBoxButtons.OK,
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

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new frmMain());
            }
        }
    }
}