using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Net;

namespace Handbrake
{
    public partial class frmUpdate : Form
    {
        public frmUpdate()
        {
            InitializeComponent();
        }

        private void btn_update_click(object sender, EventArgs e)
        {
            // Try to download the contents of the update file. If sucessful then split the 2 lines and do an update check/
            try
            {
                String updateFile = Properties.Settings.Default.updateFile;
                WebClient client = new WebClient();
                String data = client.DownloadString(updateFile);
                String[] versionData = data.Split('\n');

                lbl_GuiVersion.Text = versionData[0];
                lbl_cliVersion.Text = versionData[1];

                if ((versionData[0] != Properties.Settings.Default.GuiVersion) || (versionData[1] != Properties.Settings.Default.CliVersion))
                {
                    MessageBox.Show("A new version is available. Please visit the project website to download the update.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                }
            }
            //else fail displaying an error message.
            catch (Exception)
            {

                MessageBox.Show("Unable to check for new version at this time. Please try again later.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close(); // Close the window
        }

        private void frmUpdate_Load(object sender, EventArgs e)
        {
            // Set the labels with their correct values.
            Version.Text = Properties.Settings.Default.GuiVersion;
            cliVersion.Text = Properties.Settings.Default.CliVersion;

            // Display On/Off depending on the updateStatus setting
            string updateStatus = Properties.Settings.Default.updateStatus;

            if(updateStatus == "1"){
                lbl_startupStatus.Text = "On";
            }else{
                lbl_startupStatus.Text = "Off";
            }
        }
    }
}