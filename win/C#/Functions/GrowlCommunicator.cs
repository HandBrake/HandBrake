using System;
using System.Collections.Generic;
using System.Text;
using Growl.Connector;
using Growl.CoreLibrary;

namespace Handbrake.Functions
{
    /// <summary>
    /// Provides all functionality for communicating with Growl for Windows.
    /// </summary>
    /// <remarks>
    /// This class is implemented as a static class because:
    ///     1. It allows nearly all of the Growl-related code to be in one place
    ///     2. It prevents the main form, queue handler, and any other part of Handbrake from having to declare
    ///        or track any new instance variables
    /// </remarks>
    public static class GrowlCommunicator
    {
        /// <summary>
        /// The <see cref="GrowlConnector"/> that actually talks to Growl
        /// </summary>
        private static GrowlConnector growl;

        /// <summary>
        /// The Handbrake application instance that is registered with Growl
        /// </summary>
        private static Application application;

        /// <summary>
        /// Notification shown upon completion of encoding
        /// </summary>
        public static NotificationType EncodingComplete;


        /// <summary>
        /// Checks to see if Growl is currently running on the local machine.
        /// </summary>
        /// <returns>
        /// <c>true</c> if Growl is running;
        /// <c>false</c> otherwise
        /// </returns>
        public static bool IsRunning()
        {
            Initialize();

            return growl.IsGrowlRunning();
        }

        /// <summary>
        /// Registers Handbrake with the local Growl instance
        /// </summary>
        /// <remarks>
        /// This should usually be called at application start-up
        /// </remarks>
        public static void Register()
        {
            Initialize();

            growl.Register(application, new NotificationType[] { EncodingComplete });
        }

        /// <summary>
        /// Sends a notification to Growl. (Since Handbrake currently only supports one type of notification with
        /// static text, this is a shortcut method).
        /// </summary>
        public static void Notify()
        {
            string title = "Encoding Complete";
            string text = "Put down that cocktail...\nyour Handbrake encode is done.";
            Notification notification = new Notification(application.Name, EncodingComplete.Name, String.Empty, title, text);

            growl.Notify(notification);
        }

        /// <summary>
        /// Sends a notification to Growl. (This is the more generic version that could be used in the future if 
        /// more notification types are implemented)
        /// </summary>
        /// <param name="notificationType">The <see cref="NotificationType">type</see> of notification to send</param>
        /// <param name="title">The notification title</param>
        /// <param name="text">The notification text</param>
        /// <param name="imageUrl">The notification image as a url</param>
        public static void Notify(NotificationType notificationType, string title, string text, string imageUrl)
        {
            Notification notification = new Notification(application.Name, notificationType.Name, String.Empty, title, text);
            notification.Icon = imageUrl;

            growl.Notify(notification);
        }

        /// <summary>
        /// Initializes the GrowlCommunicator
        /// </summary>
        private static void Initialize()
        {
            if (growl == null)
            {
                growl = new GrowlConnector();
                growl.EncryptionAlgorithm = Cryptography.SymmetricAlgorithmType.PlainText;

                application = new Application("Handbrake");
                application.Icon = global::Handbrake.Properties.Resources.logo64;

                EncodingComplete = new NotificationType("Encoding Complete");
            }
        }
    }
}
