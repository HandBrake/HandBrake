using System;
using System.Collections.Generic;
using System.Threading;
using System.Diagnostics;
 

namespace Handbrake.Functions
{
    class CLI
    {
        public Process runCli(object s, string query, bool stderr, bool stdout, bool useShellExec, bool noWindow)
        {
            Process hbProc = new Process();
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
    }
}
