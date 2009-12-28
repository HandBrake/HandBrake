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
        private DVD _thisDvd;
        private Parser _readData;
        private Process _hbProc;
        private string _scanProgress;
        public event EventHandler ScanStared;
        public event EventHandler ScanCompleted;
        public event EventHandler ScanStatusChanged;

        public void ScanSource(string sourcePath, int title)
        {
            Thread t = new Thread(unused => RunScan(sourcePath, title));
            t.Start();
        }

        public DVD SouceData()
        {
            return _thisDvd;
        }

        public String LogData()
        {
            return _readData.Buffer;
        }

        public String ScanStatus()
        {
            return _scanProgress;
        }

        public Process ScanProcess()
        {
            return _hbProc;
        }

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

                _hbProc = new Process
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
                _hbProc.Start();

                _readData = new Parser(_hbProc.StandardError.BaseStream);
                _readData.OnScanProgress += new ScanProgressEventHandler(OnScanProgress);
                _thisDvd = DVD.Parse(_readData);

                // Write the Buffer out to file.
                StreamWriter scanLog = new StreamWriter(dvdInfoPath);
                scanLog.Write(_readData.Buffer);
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

        private void OnScanProgress(object sender, int currentTitle, int titleCount)
        {
            _scanProgress = string.Format("Processing Title: {0} of {1}", currentTitle, titleCount);
            if (ScanStatusChanged != null)
                ScanStatusChanged(this, new EventArgs());
        }
    }
}