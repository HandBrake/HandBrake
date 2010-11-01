/*  DownloadUpdate.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Views
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Threading;
    using System.Windows.Forms;

    /// <summary>
    /// The Download Window
    /// </summary>
    public partial class DownloadUpdate : Form
    {
        private readonly Thread downloadThread;
        private Stream responceStream;
        private Stream localStream;
        private HttpWebRequest webRequest;
        private HttpWebResponse webResponse;
        private static int progress;
        private bool killThread;

        private delegate void UpdateProgessCallback(long bytesRead, long totalBytes);

        private delegate void DownloadCompleteCallback();

        private delegate void DownloadFailedCallback();

        public DownloadUpdate(string filename)
        {
            InitializeComponent();

            this.downloadThread = new Thread(Download);
            this.downloadThread.Start(filename);
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

                this.webRequest = (HttpWebRequest)WebRequest.Create(hbUpdate);
                this.webRequest.Credentials = CredentialCache.DefaultCredentials;
                this.webResponse = (HttpWebResponse)this.webRequest.GetResponse();
                long fileSize = this.webResponse.ContentLength;

                this.responceStream = wcDownload.OpenRead(hbUpdate);
                this.localStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

                int bytesSize;
                byte[] downBuffer = new byte[2048];

                long flength = 0;
                while ((bytesSize = this.responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                {
                    if (this.killThread)
                        return;
                    this.localStream.Write(downBuffer, 0, bytesSize);
                    flength = this.localStream.Length;
                    Invoke(new UpdateProgessCallback(this.UpdateProgress), new object[] {this.localStream.Length, fileSize});
                }

                this.responceStream.Close();
                this.localStream.Close();

                if (flength != fileSize)
                    Invoke(new DownloadFailedCallback(this.DownloadFailed));
                else
                    Invoke(new DownloadCompleteCallback(this.DownloadComplete));
            }
            catch
            {
                lblProgress.Text = "An Error Occured. Please try again later.";
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

        private void BtnCancelClick(object sender, EventArgs e)
        {
            this.killThread = true;
            lblProgress.Text = "Cancelling ...";
            if (this.webResponse != null) this.webResponse.Close();
            if (this.responceStream != null) this.responceStream.Close();
            if (this.localStream != null) this.localStream.Close();
            this.Close();
        }
    }
}