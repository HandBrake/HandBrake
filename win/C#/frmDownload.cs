/*  frmDownload.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace Handbrake
{
    public partial class frmDownload : Form
    {
        private readonly Thread _downloadThread;
        private Stream _responceStream;
        private Stream _loacalStream;
        private HttpWebRequest _webRequest;
        private HttpWebResponse _webResponse;
        private static int _progress;
        private Boolean _killThread;
        private delegate void UpdateProgessCallback(Int64 bytesRead, Int64 totalBytes);
        private delegate void DownloadCompleteCallback();
        private delegate void DownloadFailedCallback();


        public frmDownload(string filename)
        {
            InitializeComponent();

            _downloadThread = new Thread(Download);
            _downloadThread.Start(filename);
        }

        private void Download(object file)
        {
            string tempPath = Path.Combine(Path.GetTempPath(), "handbrake-setup.exe");
            string hbUpdate = (string)file;
            WebClient wcDownload = new WebClient();

            try
            {
                if (File.Exists(tempPath))
                    File.Delete(tempPath);

                _webRequest = (HttpWebRequest)WebRequest.Create(hbUpdate);
                _webRequest.Credentials = CredentialCache.DefaultCredentials;
                _webResponse = (HttpWebResponse)_webRequest.GetResponse();
                Int64 fileSize = _webResponse.ContentLength;

                _responceStream = wcDownload.OpenRead(hbUpdate);
                _loacalStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

                int bytesSize;
                byte[] downBuffer = new byte[2048];

                long flength = 0;
                while ((bytesSize = _responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                {
                    if (_killThread)
                        return;
                    _loacalStream.Write(downBuffer, 0, bytesSize);
                    flength = _loacalStream.Length;
                    Invoke(new UpdateProgessCallback(this.UpdateProgress), new object[] { _loacalStream.Length, fileSize });
                }

                _responceStream.Close();
                _loacalStream.Close();

                if (flength != fileSize)
                    Invoke(new DownloadFailedCallback(this.DownloadFailed));
                else
                    Invoke(new DownloadCompleteCallback(this.DownloadComplete));
            }
            catch
            {
                // Do Nothing 
            }
        }

        private void UpdateProgress(Int64 bytesRead, Int64 totalBytes)
        {
            long p = (bytesRead * 100) / totalBytes;
            int.TryParse(p.ToString(), out _progress);
            progress_download.Value = _progress;
            lblProgress.Text = (bytesRead / 1024) + "k of " + (totalBytes / 1024) + "k ";
        }

        private void DownloadComplete()
        {
            lblProgress.Text = "Download Complete";
            btn_cancel.Text = "Close";

            Process.Start(Path.Combine(Path.GetTempPath(), "handbrake-setup.exe"));
            this.Close();
            Application.Exit();
        }

        private void DownloadFailed()
        {
            lblProgress.Text = "Download Failed";
            btn_cancel.Text = "Close";
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            _killThread = true;
            lblProgress.Text = "Cancelling ...";
            if (_webResponse != null) _webResponse.Close();
            if (_responceStream != null) _responceStream.Close();
            if (_loacalStream != null) _loacalStream.Close();
            this.Close();
        }
    }
}