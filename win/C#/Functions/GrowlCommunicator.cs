/*  GrowlCommunicator.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System;
    using Growl.Connector;

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
        private static NotificationType encodeOrQueueCompleted = new NotificationType("EncodeOrQueue", 
                                                                                      "HandBrake Status");

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
            growl.Register(application, new[] {encodeOrQueueCompleted});
        }

        /// <summary>
        /// Sends a notification to Growl. (Since Handbrake currently only supports one type of notification with
        /// static text, this is a shortcut method).
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="text">
        /// The text to display.
        /// </param>
        public static void Notify(string title, string text)
        {
            Notification notification = new Notification(application.Name, encodeOrQueueCompleted.Name, String.Empty, 
                                                         title, text);
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
            Notification notification = new Notification(application.Name, notificationType.Name, String.Empty, title, 
                                                         text)
                                            {
                                                Icon = imageUrl
                                            };

            growl.Notify(notification);
        }

        /// <summary>
        /// Initializes the GrowlCommunicator
        /// </summary>
        private static void Initialize()
        {
            if (growl == null)
            {
                growl = new GrowlConnector
                            {
                                EncryptionAlgorithm = Cryptography.SymmetricAlgorithmType.PlainText
                            };

                application = new Application("Handbrake")
                                  {
                                      Icon = Properties.Resources.logo64
                                  };
            }
        }
    }
}