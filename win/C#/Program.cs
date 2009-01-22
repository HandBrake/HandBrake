/*  Program.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

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
            // Check the system meets the system requirements.
            Boolean launch = true;
            try
            {
                // Make sure the screen resolution is not below 1024x768
                System.Windows.Forms.Screen scr = System.Windows.Forms.Screen.PrimaryScreen;
                if ((scr.Bounds.Width < 1024) || (scr.Bounds.Height < 720))
                {
                    MessageBox.Show("Your system does not meet the minimum requirements for HandBrake. \n" + "Your screen is running at: " + scr.Bounds.Width.ToString() + "x" + scr.Bounds.Height.ToString() + " \nScreen resolution is too Low. Must be 1024x720 or greater", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    launch = false;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("frmMain.cs - systemCheck() " + exc);
            }

            // Either Launch or Close the Application
            if (launch == true)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new frmMain());
            }
            else
                Application.Exit();
        }
    }

}