/*  frmDownload.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Threading;
    using System.Windows.Forms;

    /// <summary>
    /// The Update Download Screen
    /// </summary>
    public partial class frmDownload : Form
    {
        private readonly Thread downloadThread;
        private Stream responceStream;
        private Stream loacalStream;
        private HttpWebRequest webRequest;
        private HttpWebResponse webResponse;
        private static int progress;
        private bool killThread;

        private delegate void UpdateProgessCallback(long bytesRead, long totalBytes);

        private delegate void DownloadCompleteCallback();

        private delegate void DownloadFailedCallback();

        public frmDownload(string filename)
        {
            InitializeComponent();

            downloadThread = new Thread(Download);
            downloadThread.Start(filename);
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

                webRequest = (HttpWebRequest)WebRequest.Create(hbUpdate);
                webRequest.Credentials = CredentialCache.DefaultCredentials;
                webResponse = (HttpWebResponse)webRequest.GetResponse();
                long fileSize = webResponse.ContentLength;

                responceStream = wcDownload.OpenRead(hbUpdate);
                loacalStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

                int bytesSize;
                byte[] downBuffer = new byte[2048];

                long flength = 0;
                while ((bytesSize = responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                {
                    if (killThread)
                        return;
                    loacalStream.Write(downBuffer, 0, bytesSize);
                    flength = loacalStream.Length;
                    Invoke(new UpdateProgessCallback(this.UpdateProgress), new object[] {loacalStream.Length, fileSize});
                }

                responceStream.Close();
                loacalStream.Close();

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

        private void UpdateProgress(long bytesRead, long totalBytes)
        {
            long p = (bytesRead * 100) / totalBytes;
            int.TryParse(p.ToString(), out progress);
            progress_download.Value = progress;
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
            killThread = true;
            lblProgress.Text = "Cancelling ...";
            if (webResponse != null) webResponse.Close();
            if (responceStream != null) responceStream.Close();
            if (loacalStream != null) loacalStream.Close();
            this.Close();
        }
    }
}