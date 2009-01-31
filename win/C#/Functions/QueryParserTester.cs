using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake.Functions
{
    public partial class QueryParserTester : Form
    {
        public QueryParserTester()
        {
            InitializeComponent();

        }

        private void btn_test_Click(object sender, EventArgs e)
        {
            QueryParser parsed = QueryParser.Parse(rtf_query.Text);

            rtf_testContent.Clear();

            //Source
            rtf_testContent.Text += "## Source " + Environment.NewLine;
            if (parsed.DVDTitle != 0)
                rtf_testContent.Text += "Title: " + parsed.DVDTitle + Environment.NewLine;
            if (parsed.DVDChapterStart != 0)
                rtf_testContent.Text += "Start Chapter: " + parsed.DVDChapterStart + Environment.NewLine;
            if (parsed.DVDChapterFinish != 0)
                rtf_testContent.Text += "End Chapter: " + parsed.DVDChapterFinish + Environment.NewLine;

            //Output Settings
            rtf_testContent.Text += Environment.NewLine + "## Output Settings " + Environment.NewLine;
            if (parsed.Format != null)
                rtf_testContent.Text += "Format: " + parsed.Format + Environment.NewLine;
            if (parsed.LargeMP4)
                rtf_testContent.Text += "Large File Size: " + parsed.LargeMP4 + Environment.NewLine;
            if (parsed.OptimizeMP4)
                rtf_testContent.Text += "Web Optimized: " + parsed.OptimizeMP4 + Environment.NewLine;
            if (parsed.IpodAtom)
                rtf_testContent.Text += "iPod 5G Support " + parsed.IpodAtom + Environment.NewLine;

            //Picture Settings
            rtf_testContent.Text += Environment.NewLine + "## Picture Settings " + Environment.NewLine;
            if (parsed.CropValues != null)
                rtf_testContent.Text += "Cropping: " + parsed.CropValues + Environment.NewLine;
            if (parsed.Width != 0)
                rtf_testContent.Text += "Width: " + parsed.Width + Environment.NewLine;
            if (parsed.Height != 0)
                rtf_testContent.Text += "Height: " + parsed.Height + Environment.NewLine;
            if (parsed.MaxWidth != 0)
                rtf_testContent.Text += "Max Width: " + parsed.MaxWidth + Environment.NewLine;
            if (parsed.MaxHeight != 0)
                rtf_testContent.Text += "Max Height: " + parsed.MaxHeight + Environment.NewLine;
            if (parsed.Anamorphic)
                rtf_testContent.Text += "Anamorphic: " + parsed.Anamorphic + Environment.NewLine;
            if (parsed.LooseAnamorphic)
                rtf_testContent.Text += "Loose Anamorphic: " + parsed.LooseAnamorphic + Environment.NewLine;

            //Picture Settings - Filters
            rtf_testContent.Text += Environment.NewLine + "## Filters " + Environment.NewLine;
            rtf_testContent.Text += "Detelecine: " + parsed.DeTelecine + Environment.NewLine;
            rtf_testContent.Text += "Decomb: " + parsed.Decomb + Environment.NewLine;
            rtf_testContent.Text += "Deinterlace: " + parsed.DeInterlace + Environment.NewLine;
            rtf_testContent.Text += "Denoise: " + parsed.DeNoise + Environment.NewLine;
            rtf_testContent.Text += "Deblock: " + parsed.DeBlock + Environment.NewLine;

            //Video
            rtf_testContent.Text += Environment.NewLine + "## Video " + Environment.NewLine;
            rtf_testContent.Text += "Video Codec: " + parsed.VideoEncoder + Environment.NewLine;
            rtf_testContent.Text += "Video Framerate: " + parsed.VideoFramerate + Environment.NewLine;
            if (parsed.Grayscale)
                rtf_testContent.Text += "Grayscale: " + parsed.Grayscale + Environment.NewLine;
            if (parsed.TwoPass)
                rtf_testContent.Text += "2-Pass Encoding: " + parsed.TwoPass + Environment.NewLine;
            if (parsed.TurboFirstPass)
                rtf_testContent.Text += "Turbo first pass: " + parsed.TurboFirstPass + Environment.NewLine;
            if (parsed.VideoTargetSize != null)
                rtf_testContent.Text += "Target Size: " + parsed.VideoTargetSize + Environment.NewLine;
            if (parsed.AverageVideoBitrate != null)
                rtf_testContent.Text += "Avg Bitrate: " + parsed.AverageVideoBitrate + Environment.NewLine;
            if (parsed.VideoQuality != 0)
                rtf_testContent.Text += "Constant Quality: " + parsed.VideoQuality + Environment.NewLine;

            //Audio
            // TODO

            //Chapters
            if (parsed.ChapterMarkers)
            {
                rtf_testContent.Text += Environment.NewLine + "## Chapers " + Environment.NewLine;
                rtf_testContent.Text += "Chapters: " + parsed.ChapterMarkers + Environment.NewLine;
            }

            //Advanced
            if (parsed.H264Query != null)
            {
                rtf_testContent.Text += Environment.NewLine + "## x264 " + Environment.NewLine;
                rtf_testContent.Text += "x264: " + parsed.H264Query + Environment.NewLine;
            }
        }

    }
}
