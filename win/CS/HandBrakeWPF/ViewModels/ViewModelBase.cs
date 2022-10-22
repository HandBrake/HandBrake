// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewModelBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Base Class for the View Models which contains reusable code.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Diagnostics;
    using System.IO;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class ViewModelBase : PropertyChangedBase, IViewModelBase
    {
        private readonly IUserSettingService userSettingService;

        private bool hasLoaded;
        private string title;

        public ViewModelBase()
        {
            this.RelayCommand = new RelayCommand(this);
        }

        public ViewModelBase(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
            this.RelayCommand = new RelayCommand(this);
        }

        public string Title
        {
            get
            {
                return this.title;
            }

            set
            {
                this.title = value;
                this.NotifyOfPropertyChange(() => this.Title);
            }
        }

        public RelayCommand RelayCommand { get; private set; }

        public void Load()
        {
            if (!this.hasLoaded)
            {
                this.hasLoaded = true;

                // Initialise the ViewModels OnLoad method if it exists.
                this.OnLoad();
            }
        }

        public virtual void OnLoad()
        {
            // Implement in the ViewModel to perform viewmodel specific code.
        }

        public virtual void Activate()
        {
        }

        public virtual void Deactivate()
        {
        }

        public void TryClose(bool? dialogResult = null)
        {
            Type type = this.GetType().UnderlyingSystemType;
            string className = type.Name;
            string viewName = className.Replace("ViewModel", "View", StringComparison.InvariantCultureIgnoreCase);
            WindowHelper.CloseWindow(dialogResult, viewName);
        }

        public string GetMru(string key)
        {
            if (this.userSettingService == null)
            {
                throw new NotImplementedException("You must use the constructor with UserSettingService to use this function.");
            }

            string filePath = this.userSettingService.GetUserSetting<string>("mru" + key);
            if (!string.IsNullOrEmpty(filePath) && Directory.Exists(filePath))
            {
                return filePath;
            }

            // Check if the parent directory still exists.
            if (!string.IsNullOrEmpty(filePath) )
            {
                try
                {
                    DirectoryInfo parentDirectory = Directory.GetParent(filePath);
                    if (parentDirectory != null && filePath.Contains(parentDirectory.FullName))
                    {
                        return parentDirectory.FullName;
                    }
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                }
            }

            return null;
        }

        public void SetMru(string key, string value)
        {
            if (this.userSettingService == null)
            {
                throw new NotImplementedException("You must use the constructor with UserSettingService to use this function.");
            }

            if (string.IsNullOrEmpty(value) || !Directory.Exists(value))
            {
                this.userSettingService.SetUserSetting("mru" + key, string.Empty);
                return;
            }

            this.userSettingService.SetUserSetting("mru" + key, value);
        }
    }
}