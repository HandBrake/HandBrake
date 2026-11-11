// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MetaDataViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Meta Data Tab
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.IO;
    using System.Linq;
    using System.Text.Json;
    using System.Xml.Linq;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Shared;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Interfaces;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using MetadataOpenFileDialog = HandBrakeWPF.Utilities.FileDialogs.OpenFileDialog;
    using MetadataSaveFileDialog = HandBrakeWPF.Utilities.FileDialogs.SaveFileDialog;

    /// <summary>
    /// The meta data view model.
    /// </summary>
    public class MetaDataViewModel : ViewModelBase, IMetaDataViewModel
    {
        private readonly IScan scanService;
        private readonly IErrorService errorService;
        private EncodeTask task;
        private CoverArtImage coverArtImage;
        private Title currentTitle;
        private bool isNextAvailable;
        private bool isBackAvailable;
        private bool isCovertArtPreviewEnabled = true;
        private List<string> supportedMetadataKeys = new List<string>();
        private string selectedMetadataKey;

        /// <summary>
        /// Initializes a new instance of the <see cref="MetaDataViewModel"/> class. 
        /// </summary>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public MetaDataViewModel(IWindowManager windowManager, IUserSettingService userSettingService, IScan scanService, IErrorService errorService)
        {
            this.scanService = scanService;
            this.errorService = errorService;
            this.task = new EncodeTask();
            this.RemoveCommand = new SimpleRelayCommand<MetaDataValue>(this.Delete);
            this.CoverArtImage = new CoverArtImage(null);
        }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged { add { } remove { } }

        public SimpleRelayCommand<MetaDataValue> RemoveCommand { get; set; }

        public IEnumerable<string> AvailableMetadataKeys => this.supportedMetadataKeys
            .Where(key => !this.SourceMetadata.Any(row => row.Annotation == key));

        public string SelectedMetadataKey
        {
            get => this.selectedMetadataKey;
            set
            {
                this.selectedMetadataKey = value;
                this.NotifyOfPropertyChange(() => this.SelectedMetadataKey);
                this.NotifyOfPropertyChange(() => this.CanAddMetadata);
            }
        }

        public bool CanAddMetadata => this.SelectedMetadataKey != null && this.AvailableMetadataKeys.Contains(this.SelectedMetadataKey);

        public bool HasAvailableMetadataKeys => this.AvailableMetadataKeys.Any();

        public bool HasSelectedTitle => this.currentTitle != null;

        public ObservableCollection<MetaDataValue> SourceMetadata
        {
            get => this.task.MetaData;
            set
            {
                if (Equals(value, this.task.MetaData))
                {
                    return;
                }

                this.task.MetaData = value;
                this.OnPropertyChanged();
            }
        }

        public ObservableCollection<CoverArt> SourceCoverArts
        {
            get => this.task.CoverArts;
            set
            {
                if (Equals(value, this.task.CoverArts))
                {
                    return;
                }

                this.task.CoverArts = value;
                this.OnPropertyChanged();
            }
        }

        public int CoverArtIndex { get; set; }

        public CoverArtImage CoverArtImage
        {
            get => this.coverArtImage;
            set
            {
                if (Equals(value, this.coverArtImage))
                {
                    return;
                }

                this.coverArtImage = value;
                this.NotifyOfPropertyChange(() => this.CoverArtImage);
            }
        }

        public bool IsNextAvailable
        {
            get => this.isNextAvailable;
            set
            {
                if (value == this.isNextAvailable)
                {
                    return;
                }

                this.isNextAvailable = value;
                this.NotifyOfPropertyChange(() => this.IsNextAvailable);
            }
        }

        public bool IsBackAvailable
        {
            get => this.isBackAvailable;
            set
            {
                if (value == this.isBackAvailable)
                {
                    return;
                }

                this.isBackAvailable = value;
                this.NotifyOfPropertyChange(() => this.IsBackAvailable);
            }
        }

        public void NextCoverArt()
        {
            if (this.SourceCoverArts.Count > (this.CoverArtIndex + 1))
            {
                this.CoverArtIndex += 1;
                this.CoverArtImage = CreateCoverArtImage(this.SourceCoverArts[this.CoverArtIndex], this.currentTitle);
            }

            // Update the Next Button Status
            this.IsNextAvailable = this.SourceCoverArts.Count > (this.CoverArtIndex + 1);
        }

        public void PreviousCoverArt()
        {
            if (this.SourceCoverArts.Count > (this.CoverArtIndex + 1))
            {
                this.CoverArtIndex = this.CoverArtIndex - 1;
                this.CoverArtImage = CreateCoverArtImage(this.SourceCoverArts[this.CoverArtIndex], this.currentTitle);
            }

            this.IsBackAvailable = this.CoverArtIndex > 1;
        }

        public void Add()
        {
            if (this.CanAddMetadata)
            {
                this.SourceMetadata.Add(new MetaDataValue(this.SelectedMetadataKey, string.Empty) { IsNew = true });
                this.UpdateMetadataChoices();
            }
        }

        public void Delete(MetaDataValue row)
        {
            if (this.SourceMetadata.Contains(row))
            {
                this.SourceMetadata.Remove(row);
                this.UpdateMetadataChoices();
            }
        }

        public void ClearAddedFields()
        {
            foreach (MetaDataValue metadata in this.SourceMetadata.Where(metadata => metadata.IsNew).ToList())
            {
                this.SourceMetadata.Remove(metadata);
            }

            this.UpdateMetadataChoices();
        }

        public void ResetMetadata()
        {
            if (!this.HasSelectedTitle || this.errorService.ShowMessageBox(
                    Properties.Resources.MetaData_ResetPrompt,
                    Properties.Resources.AreYouSure,
                    System.Windows.MessageBoxButton.YesNo,
                    System.Windows.MessageBoxImage.Warning) != System.Windows.MessageBoxResult.Yes)
            {
                return;
            }

            this.LoadTitleMetadata();
            this.UpdateMetadataChoices();
        }

        public void ImportMetadata()
        {
            MetadataOpenFileDialog dialog = new MetadataOpenFileDialog
            {
                Filter = "Metadata files (*.json;*.xml;*.txt)|*.json;*.xml;*.txt|JSON files (*.json)|*.json|XML files (*.xml)|*.xml|Text files (*.txt)|*.txt",
                CheckFileExists = true,
                FilterIndex = 1
            };

            if (dialog.ShowDialog() != true || string.IsNullOrEmpty(dialog.FileName))
            {
                return;
            }

            try
            {
                string extension = Path.GetExtension(dialog.FileName).ToLowerInvariant();
                Dictionary<string, string> importedMetadata;
                switch (extension)
                {
                    case ".json":
                        importedMetadata = ReadJsonMetadata(dialog.FileName);
                        break;
                    case ".xml":
                        importedMetadata = ReadXmlMetadata(dialog.FileName);
                        break;
                    case ".txt":
                        importedMetadata = ReadTxtMetadata(dialog.FileName);
                        break;
                    default:
                        throw new FormatException("The selected file is not a supported metadata format.");
                }

                SortedSet<string> skippedTags = new SortedSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (KeyValuePair<string, string> importedTag in importedMetadata)
                {
                    string supportedKey = this.supportedMetadataKeys.FirstOrDefault(key => string.Equals(key, importedTag.Key, StringComparison.OrdinalIgnoreCase));
                    if (supportedKey == null)
                    {
                        skippedTags.Add(importedTag.Key);
                        continue;
                    }

                    MetaDataValue existing = this.SourceMetadata.FirstOrDefault(metadata => string.Equals(metadata.Annotation, supportedKey, StringComparison.OrdinalIgnoreCase));
                    if (existing != null)
                    {
                        existing.Value = importedTag.Value;
                    }
                    else
                    {
                        this.SourceMetadata.Add(new MetaDataValue(supportedKey, importedTag.Value) { IsNew = true });
                    }
                }

                this.UpdateMetadataChoices();
                if (skippedTags.Count > 0)
                {
                    string skippedTagList = string.Join(Environment.NewLine, skippedTags);
                    this.errorService.ShowMessageBox(
                        string.Format(Properties.Resources.MetaData_ImportSkippedTags, Environment.NewLine, skippedTagList),
                        Properties.Resources.MetaData_Import,
                        System.Windows.MessageBoxButton.OK,
                        System.Windows.MessageBoxImage.Error);
                }
            }
            catch (Exception exception)
            {
                this.errorService.ShowMessageBox(
                    string.Format(Properties.Resources.MetaData_ImportFailed, exception.Message),
                    Properties.Resources.Error,
                    System.Windows.MessageBoxButton.OK,
                    System.Windows.MessageBoxImage.Error);
            }
        }

        public void ExportMetadataFile()
        {
            MetadataSaveFileDialog dialog = new MetadataSaveFileDialog
            {
                Filter = "JSON files (*.json)|*.json|XML files (*.xml)|*.xml|Text files (*.txt)|*.txt",
                FilterIndex = 1,
                AddExtension = false,
                CheckPathExists = true,
                OverwritePrompt = false
            };

            if (dialog.ShowDialog() != true || string.IsNullOrEmpty(dialog.FileName))
            {
                return;
            }

            try
            {
                string extension;
                Action<string, Dictionary<string, string>> writeMetadata;
                switch (dialog.FilterIndex)
                {
                    case 1:
                        extension = ".json";
                        writeMetadata = WriteJsonMetadata;
                        break;
                    case 2:
                        extension = ".xml";
                        writeMetadata = WriteXmlMetadata;
                        break;
                    case 3:
                        extension = ".txt";
                        writeMetadata = WriteTxtMetadata;
                        break;
                    default:
                        throw new InvalidOperationException("The selected metadata export format is not supported.");
                }

                string filename = dialog.FileName;
                if (!string.Equals(Path.GetExtension(filename), extension, StringComparison.OrdinalIgnoreCase))
                {
                    filename = Path.ChangeExtension(filename, extension);
                }

                if (File.Exists(filename) && this.errorService.ShowMessageBox(
                        string.Format(Properties.Resources.MetaData_OverwritePrompt, filename),
                        Properties.Resources.AreYouSure,
                        System.Windows.MessageBoxButton.YesNo,
                        System.Windows.MessageBoxImage.Warning) != System.Windows.MessageBoxResult.Yes)
                {
                    return;
                }

                Dictionary<string, string> metadata = this.SourceMetadata
                    .ToDictionary(item => item.Annotation, item => item.Value ?? string.Empty);
                writeMetadata(filename, metadata);
            }
            catch (Exception exception)
            {
                this.errorService.ShowMessageBox(
                    string.Format(Properties.Resources.MetaData_ExportFailed, exception.Message),
                    Properties.Resources.Error,
                    System.Windows.MessageBoxButton.OK,
                    System.Windows.MessageBoxImage.Error);
            }
        }

        public void RefreshMetadataKeys()
        {
            var container = HandBrakeEncoderHelpers.GetContainer(EnumHelper<OutputFormat>.GetShortName(this.task.OutputFormat));
            this.supportedMetadataKeys = HandBrakeEncoderHelpers.GetMetadataKeys(container.Id);
            this.UpdateMetadataChoices();
        }

        private void UpdateMetadataChoices()
        {
            this.NotifyOfPropertyChange(() => this.AvailableMetadataKeys);
            this.NotifyOfPropertyChange(() => this.HasAvailableMetadataKeys);
            this.SelectedMetadataKey = this.AvailableMetadataKeys.FirstOrDefault();
        }

        private static void WriteJsonMetadata(string filename, Dictionary<string, string> metadata)
        {
            File.WriteAllText(filename, JsonSerializer.Serialize(metadata, new JsonSerializerOptions { WriteIndented = true }));
        }

        private static void WriteXmlMetadata(string filename, Dictionary<string, string> metadata)
        {
            XElement root = new XElement("metadata", metadata.Select(item => new XElement("tag", new XAttribute("name", item.Key), item.Value)));
            new XDocument(root).Save(filename);
        }

        private static void WriteTxtMetadata(string filename, Dictionary<string, string> metadata)
        {
            if (metadata.Values.Any(value => value.Contains('\r') || value.Contains('\n')))
            {
                throw new InvalidDataException("Plain key=value text files cannot represent values containing line breaks.");
            }

            File.WriteAllLines(filename, metadata.Select(item => item.Key + "=" + item.Value));
        }

        private static Dictionary<string, string> ReadJsonMetadata(string filename)
        {
            using JsonDocument document = JsonDocument.Parse(File.ReadAllText(filename));
            if (document.RootElement.ValueKind != JsonValueKind.Object)
            {
                throw new FormatException("The JSON root must be an object containing string-valued metadata tags.");
            }

            Dictionary<string, string> metadata = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (JsonProperty property in document.RootElement.EnumerateObject())
            {
                if (property.Value.ValueKind != JsonValueKind.String)
                {
                    throw new FormatException("Each JSON metadata value must be a string.");
                }

                metadata[property.Name] = property.Value.GetString();
            }

            return metadata;
        }

        private static Dictionary<string, string> ReadXmlMetadata(string filename)
        {
            XDocument document = XDocument.Load(filename);
            XElement root = document.Root;
            if (root == null || !string.Equals(root.Name.LocalName, "metadata", StringComparison.OrdinalIgnoreCase))
            {
                throw new FormatException("The XML document must have a metadata root element.");
            }

            Dictionary<string, string> metadata = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (XElement tag in root.Elements())
            {
                XAttribute name = tag.Attribute("name");
                if (!string.Equals(tag.Name.LocalName, "tag", StringComparison.OrdinalIgnoreCase) || name == null || string.IsNullOrWhiteSpace(name.Value))
                {
                    throw new FormatException("Each XML metadata entry must be a tag element with a non-empty name attribute.");
                }

                metadata[name.Value.Trim()] = tag.Value;
            }

            return metadata;
        }

        private static Dictionary<string, string> ReadTxtMetadata(string filename)
        {
            Dictionary<string, string> metadata = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string[] lines = File.ReadAllLines(filename);
            for (int index = 0; index < lines.Length; index++)
            {
                if (string.IsNullOrWhiteSpace(lines[index]))
                {
                    continue;
                }

                int separator = lines[index].IndexOf('=');
                if (separator <= 0 || string.IsNullOrWhiteSpace(lines[index].Substring(0, separator)))
                {
                    throw new FormatException(string.Format(Properties.Resources.MetaData_InvalidTxtLine, index + 1));
                }

                string key = lines[index].Substring(0, separator).Trim();
                string value = lines[index].Substring(separator + 1);
                metadata[key] = value;
            }

            return metadata;
        }

        /// <summary>
        /// Setup the window after a scan.
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="selectedTitle">
        /// The selected title.
        /// </param>
        /// <param name="currentPreset">
        /// The Current preset
        /// </param>
        /// <param name="encodeTask">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title selectedTitle, Preset currentPreset, EncodeTask encodeTask)
        {
            this.task = encodeTask;
            this.currentTitle = selectedTitle;
            this.NotifyOfPropertyChange(() => this.HasSelectedTitle);

            this.LoadTitleMetadata();

            this.SourceCoverArts.Clear();
            if (selectedTitle.CoverArts != null)
            {
                foreach (var item in selectedTitle.CoverArts)
                {
                    this.SourceCoverArts.Add(item);
                }
            }

            this.CoverArtImage = new CoverArtImage(null);
            if (this.SourceCoverArts != null && this.SourceCoverArts.Count > 0)
            {
                this.CoverArtIndex = this.SourceCoverArts.First().ID;
                this.CoverArtImage = CreateCoverArtImage(this.SourceCoverArts.First(), selectedTitle);
            }

            // Update the Next Button Status
            this.IsNextAvailable = this.SourceCoverArts.Count > (this.CoverArtIndex + 1);
            this.IsBackAvailable = this.CoverArtIndex > 1;

            this.NotifyOfPropertyChange(() => this.SourceMetadata);
            this.NotifyOfPropertyChange(() => this.SourceCoverArts);
            this.RefreshMetadataKeys();
        }

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="encodeTask">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask encodeTask)
        {
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="encodeTask">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask encodeTask)
        {
            this.task = encodeTask;
            this.NotifyOfPropertyChange(() => this.SourceMetadata);
            this.RefreshMetadataKeys();
        }

        public bool MatchesPreset(Preset preset)
        {
            return true;
        }

        private void LoadTitleMetadata()
        {
            this.SourceMetadata.Clear();
            if (this.currentTitle.Metadata != null)
            {
                foreach (var item in this.currentTitle.Metadata)
                {
                    this.SourceMetadata.Add(new MetaDataValue(item.Key, item.Value));
                }
            }
        }

        private CoverArtImage CreateCoverArtImage(CoverArt covertArt, Title selectedTitle)
        {
            if (isCovertArtPreviewEnabled)
            {
                CoverArtImage newImage = new CoverArtImage(covertArt);
                newImage.Image = this.scanService.GetCoverArt(covertArt, selectedTitle.TitleNumber);
                this.CoverArtImage = newImage;

                return newImage;
            }

            return new CoverArtImage(null);
        }

        public Func<RawCoverArtData> CreateCoverArtRawLoader(CoverArt coverArt)
        {
            int titleNumber = this.currentTitle.TitleNumber;
            return () => this.scanService.GetCoverArtRaw(coverArt, titleNumber);
        }
    }
}
