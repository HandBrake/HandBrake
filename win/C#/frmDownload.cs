using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace Handbrake
{
    public partial class frmDownload : Form
    {
        private Thread downloadThread;
        private Stream responceStream;
        private Stream loacalStream;
        private HttpWebRequest webRequest;
        private HttpWebResponse webResponse;
        private static int progress;
        private delegate void UpdateProgessCallback(Int64 BytesRead, Int64 TotalBytes);
        private delegate void DownloadCompleteCallback();
        private delegate void DownloadFailedCallback();


        public frmDownload()
        {
            InitializeComponent();

            try
            {
                downloadThread = new Thread(Download);
                downloadThread.Start();
            }
            catch (Exception exc)
            {
                MessageBox.Show("An error occured on the Download Thread \n" + exc.ToString(),"Error",MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
          
        }

        private void Download()
        {
            Functions.RssReader rssRead = new Functions.RssReader();

            string appPath = Application.StartupPath.ToString() + "\\";
            string hbUpdate = rssRead.downloadFile(); 
            string downloadPath = appPath + "Handbrake-win.exe";
            WebClient wcDownload = new WebClient();
                try
                {
                    webRequest = (HttpWebRequest)WebRequest.Create(hbUpdate);
                    webRequest.Credentials = CredentialCache.DefaultCredentials;
                    webResponse = (HttpWebResponse)webRequest.GetResponse();
                    Int64 fileSize = webResponse.ContentLength;

                    responceStream = wcDownload.OpenRead(hbUpdate);
                    loacalStream = new FileStream(downloadPath, FileMode.Create, FileAccess.Write, FileShare.None);

                    int bytesSize = 0;
                    byte[] downBuffer = new byte[2048];

                    long flength = 0;
                    while ((bytesSize = responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                    {
                        loacalStream.Write(downBuffer, 0, bytesSize);
                        flength = loacalStream.Length;
                        this.Invoke(new UpdateProgessCallback(this.UpdateProgress), new object[] { loacalStream.Length, fileSize });
                    }

                    responceStream.Close();
                    loacalStream.Close();

                    if (flength != fileSize)
                        this.Invoke(new DownloadFailedCallback(this.downloadFailed));
                    else
                        this.Invoke(new DownloadCompleteCallback(this.downloadComplete));
                }
                catch (Exception)
                {
                    // Do Nothing 
                }
        }

        private void UpdateProgress(Int64 BytesRead, Int64 TotalBytes)
        {
            try
            {
                long p = (BytesRead * 100) / TotalBytes;
                progress = int.Parse(p.ToString());
                progress_download.Value = progress;
                lblProgress.Text = (BytesRead / 1024) + "k of " + (TotalBytes / 1024) + "k ";
            }
            catch (Exception exc)
            {
                MessageBox.Show("Integer Convertion Error On Download \n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void downloadComplete()
        {
            lblProgress.Text = "Download Complete";
            btn_cancel.Text = "Close";

            string appPath = Application.StartupPath.ToString() + "\\";
            
            Process hbproc = Process.Start(appPath + "Handbrake-win.exe");
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();

            this.Close();
        }

        private void downloadFailed()
        {
            lblProgress.Text = "Download Failed";
            btn_cancel.Text = "Close";
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            webResponse.Close();
            responceStream.Close();
            loacalStream.Close();
            downloadThread.Abort();
            progress_download.Value = 0;
            lblProgress.Text = "Download Stopped";
            this.Close();
        }
    }
}