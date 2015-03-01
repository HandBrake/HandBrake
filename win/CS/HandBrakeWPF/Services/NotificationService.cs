// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NotificationService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Notification Service (Growl Connector)
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;

    using IQueueProcessor = HandBrakeWPF.Services.Interfaces.IQueueProcessor;

    /// <summary>
    /// The Notification Service (Growl Connector)
    /// </summary>
    public class NotificationService : INotificationService
    {
        /// <summary>
        /// The user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// Initializes a new instance of the <see cref="NotificationService"/> class.
        /// </summary>
        /// <param name="encodeService">
        /// The encode Service.
        /// </param>
        /// <param name="queueProcessor">
        /// The queue Processor.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public NotificationService(IEncode encodeService, IQueueProcessor queueProcessor, IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
           // encodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;
           // queueProcessor.QueueCompleted += this.QueueProcessorQueueCompleted;
        }

        /// <summary>
        /// The queue processor_ queue completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void QueueProcessorQueueCompleted(object sender, System.EventArgs e)
        {
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlQueue))
            {
               // GrowlCommunicator.Notify("Queue Completed", "Put down that cocktail...\nyour Handbrake queue is done.");
            }
        }

        /// <summary>
        /// The encode service_ encode completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeCompletedEventArgs.
        /// </param>
        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlEncode))
            {
               // GrowlCommunicator.Notify("Encode Completed", "Put down that cocktail...\nyour Handbrake encode is done.");
            }
        }
    }
}
