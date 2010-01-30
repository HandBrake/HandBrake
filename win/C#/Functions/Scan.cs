/*  Scan.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Threading;
using Handbrake.Parsing;

namespace Handbrake.Functions
{
    public class Scan
    {
        private DVD ThisDvd;
        private Parser ReadData;
        private Process HbProc;
        private string ScanProgress;

        /// <summary>
        /// Scan has Started
        /// </summary>
        public event EventHandler ScanStared;

        /// <summary>
        /// Scan has completed
        /// </summary>
        public event EventHandler ScanCompleted;

        /// <summary>
        /// Scan process has changed to a new title
        /// </summary>
        public event EventHandler ScanStatusChanged;

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">Path to the file to scan</param>
        /// <param name="title">int title number. 0 for scan all</param>
        public void ScanSource(string sourcePath, int title)
        {
            Thread t = new Thread(unused => RunScan(sourcePath, title));
            t.Start();
        }

        /// <summary>
        /// Object containing the information parsed in the scan.
        /// </summary>
        /// <returns></returns>
        public DVD SouceData()
        {
            return ThisDvd;
        }

        /// <summary>
        /// Raw log output from HandBrake CLI
        /// </summary>
        /// <returns></returns>
        public String LogData()
        {
            return ReadData.Buffer;
        }

        /// <summary>
        /// Progress of the scan.
        /// </summary>
        /// <returns></returns>
        public String ScanStatus()
        {
            return ScanProgress;
        }

        /// <summary>
        /// The Scan Process
        /// </summary>
        /// <returns></returns>
        public Process ScanProcess()
        {
            return HbProc;
        }

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePath"></param>
        /// <param name="title"></param>
        private void RunScan(object sourcePath, int title)
        {
            try
            {
                if (ScanStared != null)
                    ScanStared(this, new EventArgs());

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
                string dvdInfoPath = Path.Combine(logDir, "last_scan_log.txt");

                // Make we don't pick up a stale last_encode_log.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                    File.Delete(dvdInfoPath);

                String dvdnav = string.Empty;
                if (Properties.Settings.Default.noDvdNav)
                    dvdnav = " --no-dvdnav";

                HbProc = new Process
                {
                    StartInfo =
                    {
                        FileName = handbrakeCLIPath,
                        Arguments = String.Format(@" -i ""{0}"" -t{1} {2} -v ", sourcePath, title, dvdnav),
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        UseShellExecute = false,
                        CreateNoWindow = true
                    }
                };
                HbProc.Start();

                ReadData = new Parser(HbProc.StandardError.BaseStream);
                ReadData.OnScanProgress += new ScanProgressEventHandler(OnScanProgress);
                ThisDvd = DVD.Parse(ReadData);

                // Write the Buffer out to file.
                StreamWriter scanLog = new StreamWriter(dvdInfoPath);
                scanLog.Write(ReadData.Buffer);
                scanLog.Flush();
                scanLog.Close();

                if (ScanCompleted != null)
                    ScanCompleted(this, new EventArgs());
            }
            catch (Exception exc)
            {
                Console.WriteLine("frmMain.cs - scanProcess() " + exc);
            }
        }

        /// <summary>
        /// Fire an event when the scan process progresses
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="currentTitle"></param>
        /// <param name="titleCount"></param>
        private void OnScanProgress(object sender, int currentTitle, int titleCount)
        {
            ScanProgress = string.Format("Processing Title: {0} of {1}", currentTitle, titleCount);
            if (ScanStatusChanged != null)
                ScanStatusChanged(this, new EventArgs());
        }
    }
}