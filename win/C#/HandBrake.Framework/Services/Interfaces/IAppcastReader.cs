namespace HandBrake.Framework.Services.Interfaces
{
    using System;

    public interface IAppcastReader
    {
        /// <summary>
        /// Gets Information about an update to HandBrake
        /// </summary>
        Uri DescriptionUrl { get; }

        /// <summary>
        /// Gets HandBrake's version from the appcast.xml file.
        /// </summary>
        string Version { get; }

        /// <summary>
        /// Gets HandBrake's Build from the appcast.xml file.
        /// </summary>
        string Build { get; }

        /// <summary>
        /// Gets the URL for update file.
        /// </summary>
        string DownloadFile { get; }

        /// <summary>
        /// Get the build information from the required appcasts. Run before accessing the public vars.
        /// </summary>
        /// <param name="input">
        /// The input.
        /// </param>
        void GetUpdateInfo(string input);
    }
}