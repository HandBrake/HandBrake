/*  MainViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;
    using System.Diagnostics;
    using System.IO;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    [Export(typeof(IMainViewModel))]
    public class MainViewModel : ViewModelBase, IMainViewModel
    {
        #region Private Variables and Services

        /// <summary>
        /// The Source Scan Service.
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        /// The Encode Service
        /// </summary>
        private readonly IQueueProcessor queueProcessor;

        /// <summary>
        /// The preset service
        /// </summary>
        private readonly IPresetService presetService;

        /// <summary>
        /// HandBrakes Main Window Title
        /// </summary>
        private string windowName;

        /// <summary>
        /// The Source Label
        /// </summary>
        private string sourceLabel;

        /// <summary>
        /// The Toolbar Status Label
        /// </summary>
        private string programStatusLabel;

        #endregion

        #region Properties

        [ImportingConstructor]
        public MainViewModel(IWindowManager windowManager) : base(windowManager) 
        {
            // Setup Services (TODO - Bring Castle back into the project to wire these up for us)
            this.scanService = File.Exists("hb.dll") ? (IScan)new LibScan() : new ScanService();
            this.queueProcessor = new QueueProcessor(Process.GetProcessesByName("HandBrake").Length);
            this.presetService = new PresetService();

            // Setup Properties
            this.WindowTitle = "HandBrake WPF Test Application";

            // Setup Events
            this.scanService.ScanStared += this.ScanStared;
            this.scanService.ScanCompleted += this.ScanCompleted;
            this.scanService.ScanStatusChanged += this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted += this.QueueCompleted;
            this.queueProcessor.QueuePaused += this.QueuePaused;
            this.queueProcessor.EncodeService.EncodeStarted += this.EncodeStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged += this.EncodeStatusChanged;
        }

        /// <summary>
        /// Gets or sets TestProperty.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return this.windowName;
            }

            set
            {
                if (!object.Equals(this.windowName, value))
                {
                    this.windowName = value;
                }
            }
        }

        /// <summary>
        /// Gets a list of presets
        /// </summary>
        public ObservableCollection<Preset> Presets
        {
            get
            {
                return this.presetService.Presets;
            }
        }

        /// <summary>
        /// Gets or sets The Current Encode Task that the user is building
        /// </summary>
        public EncodeTask CurrentTask { get; set; }

        /// <summary>
        /// Gets or sets the Last Scanned Source
        /// This object contains information about the scanned source.
        /// </summary>
        public Source ScannedSource { get; set; }

        /// <summary>
        /// Gets or sets the Source Label
        /// This indicates the status of scans.
        /// </summary>
        public string SourceLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.sourceLabel) ? "Select 'Source' to continue" : this.sourceLabel;
            }

            set
            {
                if (!object.Equals(this.sourceLabel, value))
                {
                    this.sourceLabel = value;
                }
            }
        }

        /// <summary>
        /// Gets or sets the Program Status Toolbar Label
        /// This indicates the status of HandBrake
        /// </summary>
        public string ProgramStatusLabel
        {
            get
            {
                return string.IsNullOrEmpty(this.programStatusLabel) ? "Ready" : this.sourceLabel;
            }

            set
            {
                if (!object.Equals(this.programStatusLabel, value))
                {
                    this.programStatusLabel = value;
                }
            }
        }

        #endregion

        /// <summary>
        /// Shutdown this View
        /// </summary>
        public void Shutdown()
        {
            // Unsubscribe from Events.
            this.scanService.ScanStared -= this.ScanStared;
            this.scanService.ScanCompleted -= this.ScanCompleted;
            this.scanService.ScanStatusChanged -= this.ScanStatusChanged;

            this.queueProcessor.QueueCompleted -= this.QueueCompleted;
            this.queueProcessor.QueuePaused -= this.QueuePaused;
            this.queueProcessor.EncodeService.EncodeStarted -= this.EncodeStarted;
            this.queueProcessor.EncodeService.EncodeStatusChanged -= this.EncodeStatusChanged;
        }


        #region Menu and Taskbar
        
        public void AboutApplication()
        {
        }
        
        /// <summary>
        /// Shutdown the Application
        /// </summary>
        public void ExitApplication()
        {
            Application.Current.Shutdown();
        }

        #endregion


        #region Event Handlers
        /// <summary>
        /// Handle the Scan Status Changed Event.
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.ScanProgressEventArgs e)
        {
            this.SourceLabel = "Scanning Title " + e.CurrentTitle + " of " + e.Titles;
        }

        /// <summary>
        /// Handle the Scan Completed Event
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanCompleted(object sender, HandBrake.ApplicationServices.EventArgs.ScanCompletedEventArgs e)
        {
            if (e.Successful)
            {
                this.ScannedSource = this.scanService.SouceData;
            }
        }

        /// <summary>
        /// Handle the Scan Started Event
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void ScanStared(object sender, EventArgs e)
        {
            // TODO - Disable relevant parts of the UI.
        }

        /// <summary>
        /// The Encode Status has changed Handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The Encode Progress Event Args
        /// </param>
        private void EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
        {
            //
        }

        /// <summary>
        /// Encode Started Handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void EncodeStarted(object sender, EventArgs e)
        {
            // TODO Handle Updating the UI
        }

        /// <summary>
        /// The Queue has been paused handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void QueuePaused(object sender, EventArgs e)
        {
            // TODO Handle Updating the UI
        }

        /// <summary>
        /// The Queue has completed handler
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void QueueCompleted(object sender, EventArgs e)
        {
            // TODO Handle Updating the UI
        }
        #endregion
    }
}