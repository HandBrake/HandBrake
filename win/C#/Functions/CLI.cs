using System;
using System.Collections.Generic;
using System.Threading;
using System.Diagnostics;
using System.Windows.Forms;
 

namespace Handbrake.Functions
{
    class CLI
    {
        Process hbProc = new Process();
        
        public Process runCli(object s, string query, bool stderr, bool stdout, bool useShellExec, bool noWindow)
        {
            hbProc.StartInfo.FileName = "hbcli.exe";
            hbProc.StartInfo.Arguments = query;
            hbProc.StartInfo.RedirectStandardOutput = stdout;
            hbProc.StartInfo.RedirectStandardError = stderr;
            hbProc.StartInfo.UseShellExecute = useShellExec;
            hbProc.StartInfo.CreateNoWindow = noWindow;
            hbProc.Start();     

            // Set the process Priority
            switch (Properties.Settings.Default.processPriority)
            {
                case "Realtime":
                    hbProc.PriorityClass = ProcessPriorityClass.RealTime;
                    break;
                case "High":
                    hbProc.PriorityClass = ProcessPriorityClass.High;
                    break;
                case "Above Normal":
                    hbProc.PriorityClass = ProcessPriorityClass.AboveNormal;
                    break;
                case "Normal":
                    hbProc.PriorityClass = ProcessPriorityClass.Normal;
                    break;
                case "Low":
                    hbProc.PriorityClass = ProcessPriorityClass.Idle;
                    break;
                default:
                    hbProc.PriorityClass = ProcessPriorityClass.BelowNormal;
                    break;
            }
            return hbProc;
        }

        public void killCLI()
        {
            try
            {
                hbProc.Kill();
            }
            catch (Exception)
            {
                // No need to do anything. Chances are the process was already dead.
            }
        }

        public void closeCLI()
        {
            hbProc.Close();
            hbProc.Dispose();
        }
    }
}
