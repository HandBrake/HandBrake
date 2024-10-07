// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UserSettingService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The User Setting Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text.Json;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Video;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using GeneralApplicationException = HandBrake.App.Core.Exceptions.GeneralApplicationException;
    using SettingChangedEventArgs = EventArgs.SettingChangedEventArgs;
    using SystemInfo = Utilities.SystemInfo;

    /// <summary>
    /// The User Setting Service
    /// </summary>
    public class UserSettingService : IUserSettingService
    {
        private readonly ILog logService;

        private readonly string settingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly()), "settings.json");
        private readonly string releaseSettingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(false), "settings.json");
        private readonly string nightlySettingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(true), "settings.json");
        private Dictionary<string, object> userSettings;

        /// <summary>
        /// Initializes a new instance of the <see cref="UserSettingService"/> class.
        /// </summary>
        public UserSettingService(ILog logService)
        {
            this.logService = logService;
            this.Load();
        }

        /// <summary>
        /// The setting changed.
        /// </summary>
        public event SettingEventHandler SettingChanged;

        /// <summary>
        /// Set the specified user setting.
        /// </summary>
        /// <param name="name">
        /// Name of the property
        /// </param>
        /// <param name="value">
        /// The value to store.
        /// </param>
        public void SetUserSetting(string name, object value)
        {
            this.userSettings[name] = value;
            this.Save();

            this.OnSettingChanged(new SettingChangedEventArgs { Key = name, Value = value });
        }

        /// <summary>
        /// Get user setting for a given key.
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <typeparam name="T">
        /// The Type of the setting
        /// </typeparam>
        /// <returns>
        /// The user setting
        /// </returns>
        public T GetUserSetting<T>(string name)
        {
            try
            {
                if (this.userSettings.ContainsKey(name))
                {
                    object settingValue = this.userSettings[name];
                    if (settingValue is JsonElement)
                    {
                        T rawValue = JsonSerializer.Deserialize<T>(
                            ((JsonElement)settingValue).GetRawText(),
                            JsonSettings.Options);
                        return rawValue;
                    }

                    T objectElement = (T)this.userSettings[name];
                    return objectElement;
                }
            }
            catch (Exception e)
            {
                this.logService.LogMessage("Unable to fetch user setting:  " + name + " - " + e);
                return GetDefaultValue<T>(name);
            }

            return default(T);
        }

        public void ResetSettingsToDefaults()
        {
            this.userSettings.Clear();

            // Set Defaults
            Dictionary<string, object> defaults = this.GetDefaults();
            foreach (var item in defaults.Where(item => !this.userSettings.Keys.Contains(item.Key)))
            {
                this.userSettings.Add(item.Key, item.Value);
                this.Save();
            }
        }

        /// <summary>
        /// The on setting changed.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected virtual void OnSettingChanged(SettingChangedEventArgs e)
        {
            SettingEventHandler handler = this.SettingChanged;
            if (handler != null)
            {
                handler(this, e);
            }
        }

        /// <summary>
        /// Save the User Settings
        /// </summary>
        private void Save()
        {
            try
            {
                string directory = Path.GetDirectoryName(this.settingsFile);
                if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                using (StreamWriter file = new StreamWriter(new FileStream(this.settingsFile, FileMode.Create, FileAccess.Write)))
                {
                    string appSettings = JsonSerializer.Serialize(this.userSettings, JsonSettings.Options);
                    file.Write(appSettings);
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    Resources.UserSettings_AnErrorOccurred,
                    Resources.SettingService_SaveErrorReset,
                    exc);
            }
        }

        /// <summary>
        /// Load the User Settings
        /// </summary>
        private void Load()
        {
            try
            {
                // Load up the users current settings file.
                if (File.Exists(this.settingsFile))
                {
                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        string appSettings = reader.ReadToEnd();
                        Dictionary<string, object> deserializedSettings = JsonSerializer.Deserialize<Dictionary<string, object>>(appSettings, JsonSettings.Options);

                        this.userSettings = deserializedSettings;
                    }
                }
                else if (HandBrakeVersionHelper.IsNightly() && File.Exists(this.releaseSettingsFile))
                {
                    // Port the release versions config to the nightly.
                    if (!Directory.Exists(DirectoryUtilities.GetUserStoragePath(true)))
                    {
                        Directory.CreateDirectory(DirectoryUtilities.GetUserStoragePath(true));
                    }

                    File.Copy(this.releaseSettingsFile, this.nightlySettingsFile);

                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        string appSettings = reader.ReadToEnd();
                        Dictionary<string, object> deserializedSettings = JsonSerializer.Deserialize<Dictionary<string, object>>(appSettings, JsonSettings.Options);
                        this.userSettings = deserializedSettings;
                    }
                }
                else
                {
                    this.userSettings = new Dictionary<string, object>();
                }

                // Add any missing / new settings
                Dictionary<string, object> defaults = this.GetDefaults();
                foreach (var item in defaults.Where(item => !this.userSettings.Keys.Contains(item.Key)))
                {
                    this.userSettings.Add(item.Key, item.Value);
                    this.Save();
                }

                this.ResetUnsupportedSettings();
            }
            catch (Exception exc)
            {
                try
                {
                    this.userSettings = this.GetDefaults();
                    if (File.Exists(this.settingsFile))
                    {
                        File.Delete(this.settingsFile);
                    }

                    this.Save();

                    throw new GeneralApplicationException(Resources.UserSettings_YourSettingsHaveBeenReset, Resources.UserSettings_YourSettingsAreCorrupt, exc);
                }
                catch (Exception)
                {
                    throw new GeneralApplicationException(string.Format(Resources.UserSettings_UnableToLoad, this.settingsFile), Resources.UserSettings_UnableToLoadSolution, exc);
                }
            }
        }

        private void ResetUnsupportedSettings()
        {
            // Legacy Settings forced Reset.
            this.userSettings[UserSettingConstants.ScalingMode] = VideoScaler.Lanczos;

            if (SystemInfo.MaximumSimultaneousInstancesSupported < 2)
            {
                this.userSettings[UserSettingConstants.ProcessIsolationEnabled] = false;
                this.userSettings[UserSettingConstants.SimultaneousEncodes] = 1;
            }

            // Handle change of language code zh to zh-CN
            object language;
            if (this.userSettings.TryGetValue(UserSettingConstants.UiLanguage, out language))
            {
                if (language is string and "zh")
                {
                    // Reset to use system language if we have the old zh stored in settings.
                    this.userSettings[UserSettingConstants.UiLanguage] = InterfaceLanguageUtilities.UseSystemLanguage;
                }
            }
        }
        
        /// <summary>
        /// Load Default Settings
        /// </summary>
        /// <returns>
        /// The get defaults.
        /// </returns>
        private Dictionary<string, object> GetDefaults()
        {
            Dictionary<string, object> defaults = new Dictionary<string, object>();

            defaults.Add(UserSettingConstants.Verbosity, 1);

            // General
            defaults.Add(UserSettingConstants.UpdateStatus, false);
            defaults.Add(UserSettingConstants.LastUpdateCheckDate, DateTime.Now.Date.AddDays(-30));
            defaults.Add(UserSettingConstants.DaysBetweenUpdateCheck, 1);
            defaults.Add(UserSettingConstants.DarkThemeMode, DarkThemeMode.Light);
            defaults.Add(UserSettingConstants.ShowPreviewOnSummaryTab, true);
            defaults.Add(UserSettingConstants.MainWindowMinimize, false);
            defaults.Add(UserSettingConstants.ClearCompletedFromQueue, false);
            defaults.Add(UserSettingConstants.ShowStatusInTitleBar, false);
            defaults.Add(UserSettingConstants.ShowAddAllToQueue, false);
            defaults.Add(UserSettingConstants.ShowAddSelectionToQueue, false);
            defaults.Add(UserSettingConstants.MediaPlayerPath, @"C:\Program Files\VideoLAN\vlc\vlc.exe");
            defaults.Add(UserSettingConstants.PresetMenuDisplayMode, 0);
            defaults.Add(UserSettingConstants.RightToLeftUi, 0); 
            
            // Output Files
            defaults.Add(UserSettingConstants.AutoNaming, true);
            defaults.Add(UserSettingConstants.AutoNamePath, string.Empty);
            defaults.Add(UserSettingConstants.AutoNameFormat, "{source}");
            defaults.Add(UserSettingConstants.AutonameFilePrePostString, "output_");
            defaults.Add(UserSettingConstants.AutoNameTitleCase, true);
            defaults.Add(UserSettingConstants.AutoNameRemoveUnderscore, true);
            defaults.Add(UserSettingConstants.AutonameFileCollisionBehaviour, 0);
            defaults.Add(UserSettingConstants.UseIsoDateFormat, false);
            defaults.Add(UserSettingConstants.AlwaysUseDefaultPath, true);
            defaults.Add(UserSettingConstants.RemovePunctuation, false);
            defaults.Add(UserSettingConstants.FileOverwriteBehaviour, 0);
            defaults.Add(UserSettingConstants.UseM4v, 0);

            // When Done
            defaults.Add(UserSettingConstants.SendFile, false);
            defaults.Add(UserSettingConstants.WhenCompleteAction, 0);
            defaults.Add(UserSettingConstants.WhenDonePerformActionImmediately, false);
            defaults.Add(UserSettingConstants.PlaySoundWhenDone, false);
            defaults.Add(UserSettingConstants.PlaySoundWhenQueueDone, false);
            defaults.Add(UserSettingConstants.WhenDoneAudioFile, string.Empty);

            // Video
            bool intelDefaultSetting = HandBrakeHardwareEncoderHelper.IsQsvAvailable;
            bool nvidiaDefaultSetting = HandBrakeHardwareEncoderHelper.IsNVEncH264Available;

            defaults.Add(UserSettingConstants.EnableQuickSyncDecoding, intelDefaultSetting);
            defaults.Add(UserSettingConstants.EnableQuickSyncHyperEncode, intelDefaultSetting);
            defaults.Add(UserSettingConstants.UseQSVDecodeForNonQSVEnc, false);
            defaults.Add(UserSettingConstants.EnableNvDecSupport, nvidiaDefaultSetting);
            defaults.Add(UserSettingConstants.EnableQuickSyncLowPower, true);
            defaults.Add(UserSettingConstants.EnableDirectXDecoding, SystemInfo.IsArmDevice);

            // Advanced
            defaults.Add(UserSettingConstants.PreventSleep, true);
            defaults.Add(UserSettingConstants.PauseEncodingOnLowBattery, true);
            defaults.Add(UserSettingConstants.LowBatteryLevel, 15);

            defaults.Add(UserSettingConstants.DisableLibDvdNav, false);
            defaults.Add(UserSettingConstants.PauseOnLowDiskspace, true);
            defaults.Add(UserSettingConstants.PauseQueueOnLowDiskspaceLevel, 2000000000L);
            defaults.Add(UserSettingConstants.PreviewScanCount, 10);
            defaults.Add(UserSettingConstants.MinScanDuration, 10);
            defaults.Add(UserSettingConstants.MaxScanDuration, 0);
            defaults.Add(UserSettingConstants.KeepDuplicateTitles, false);
            defaults.Add(UserSettingConstants.ProcessPriorityInt, 2);
            defaults.Add(UserSettingConstants.X264Step, 0.5);
            defaults.Add(UserSettingConstants.SaveLogToCopyDirectory, false);
            defaults.Add(UserSettingConstants.SaveLogWithVideo, false);
            defaults.Add(UserSettingConstants.ClearOldLogs, true);

            defaults.Add(UserSettingConstants.ExcludedExtensions, new List<string> { "png", "jpg", "srt", "ass", "ssa", "txt" });
            defaults.Add(UserSettingConstants.RecursiveFolderScan, false);
            
            // Preview
            defaults.Add(UserSettingConstants.LastPreviewDuration, 30);
            defaults.Add(UserSettingConstants.UseExternalPlayer, false);

            // Experimental
            defaults.Add(UserSettingConstants.ProcessIsolationEnabled, true);
            defaults.Add(UserSettingConstants.ProcessIsolationPort, 8037);
            defaults.Add(UserSettingConstants.SimultaneousEncodes, 1);

            // Misc
            defaults.Add(UserSettingConstants.ScalingMode, 0);
            defaults.Add(UserSettingConstants.ForcePresetReset, 3);
            defaults.Add(UserSettingConstants.PreviewShowPictureSettingsOverlay, false);
            defaults.Add(UserSettingConstants.RunCounter, 0);
            defaults.Add(UserSettingConstants.ForceSoftwareRendering, false);
            defaults.Add(UserSettingConstants.IsUpdateAvailableBuild, 0);
            defaults.Add(UserSettingConstants.ExtendedQueueDisplay, true);
            defaults.Add(UserSettingConstants.HardwareDetectTimeoutSeconds, 12);
            defaults.Add(UserSettingConstants.SimpleQueueView, false);
            defaults.Add(UserSettingConstants.ShowPresetDesc, true);
            defaults.Add(UserSettingConstants.IsLegacyMenuShown, true);

            return defaults;
        }

        private T GetDefaultValue<T>(string name)
        {
            try
            {
                // If the current setting is corrupt, return the default and log it.
                Dictionary<string, object> defaults = this.GetDefaults();
                if (defaults.ContainsKey(name))
                {
                    return (T)this.userSettings[name];
                }
            }
            catch (Exception e)
            {
                this.logService.LogMessage("Unable to fetch default setting:  " + name + " - " + e);
                return default(T);
            }

            return default(T);
        }
    }
}