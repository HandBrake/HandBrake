namespace HandBrake.ApplicationServices
{
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop;

    /// <summary>
    /// Tempory Class which manages services until Windosor is added back into the project to handle it for us.
    /// </summary>
    public class ServiceManager
    {
        /// <summary>
        /// Backing Field for the User Setting Service.
        /// </summary>
        private static IUserSettingService userSettingService;

        /// <summary>
        /// The Backing field for HandBrake Instance.
        /// </summary>
        private static HandBrakeInstance handBrakeInstance;

        /// <summary>
        /// Gets UserSettingService.
        /// </summary>
        public static IUserSettingService UserSettingService
        {
            get
            {
                return userSettingService ?? (userSettingService = new UserSettingService());
            }
        }

        /// <summary>
        /// Gets HandBrakeInstance.
        /// </summary>
        public static HandBrakeInstance HandBrakeInstance
        {
            get
            {
                return handBrakeInstance ?? (handBrakeInstance = new HandBrakeInstance());
            }
        }
    }
}
