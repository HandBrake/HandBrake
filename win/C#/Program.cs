/*  Program.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.IO;
using Handbrake.Presets;

namespace Handbrake
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Screen scr = Screen.PrimaryScreen;
            if ((scr.Bounds.Width < 1024) || (scr.Bounds.Height < 620))
                MessageBox.Show("Your system does not meet the minimum requirements for HandBrake. \n" + "Your screen is running at: " + scr.Bounds.Width + "x" + scr.Bounds.Height + " \nScreen resolution is too Low. Must be 1024x720 or greater", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
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