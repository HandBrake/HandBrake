using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Runtime.InteropServices;


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
            
            /* Some Code to allow development builds to expire.
             * 
             * long start = DateTime.Now.Ticks;
             * 633286573227430160 today was long end = DateTime.Now.AddDays(31).Ticks;
             * if (start > 633286573227430160) { MessageBox.Show("Sorry, this Handbrake has expired"); return; }
             * 
             */


            // Check the system meets the system requirements.
            Boolean launch = true;
            try
            {
                // Make sure the screen resolution is not below 1024x768
                System.Windows.Forms.Screen scr = System.Windows.Forms.Screen.PrimaryScreen;
                if ((scr.Bounds.Width < 1024) || (scr.Bounds.Height < 768))
                {
                    MessageBox.Show("Your system does not meet the minimum requirements for HandBrake. \n Screen resolution is too Low. Must be 1024x768 or greater", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    launch = false;
                }

                // Make sure the system has enough RAM. 384MB or greater
                uint memory = MemoryCheck.CheckMemeory();

                if (memory < 319) // Set to 319 to allow for 64MB dedicated to video Memory and Windows returnig the memory figure slightly out.
                {
                    MessageBox.Show("Your system does not meet the minimum requirements for HandBrake. \n Insufficient Memory. 384MB or greater required.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    launch = false;
                }
            }
            catch (Exception exc)
            {
                if (Properties.Settings.Default.GuiDebug == "Checked")
                {
                    MessageBox.Show("frmMain.cs - systemCheck() " + exc.ToString());
                }
            }


            // Either Launch or Close the Application
            if (launch == true)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new frmMain());
            }
            else
            {
                Application.Exit();
            }
        }
    }

    class MemoryCheck
    {
        public struct MEMORYSTATUS
        {
            public UInt32 dwLength;
            public UInt32 dwMemoryLoad;
            public UInt32 dwTotalPhys; // Used
            public UInt32 dwAvailPhys;
            public UInt32 dwTotalPageFile;
            public UInt32 dwAvailPageFile;
            public UInt32 dwTotalVirtual;
            public UInt32 dwAvailVirtual;
            // Aditional Varibles left in for future usage (JIC)
        }

        [DllImport("kernel32.dll")]
        public static extern void GlobalMemoryStatus
        (
            ref MEMORYSTATUS lpBuffer
        );

        public static uint CheckMemeory()
        {
            // Call the native GlobalMemoryStatus method
            // with the defined structure.
            MEMORYSTATUS memStatus = new MEMORYSTATUS();
            GlobalMemoryStatus(ref memStatus);

            uint MemoryInfo = memStatus.dwTotalPhys;

            return MemoryInfo;
        }
    }
}