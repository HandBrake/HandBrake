namespace HandBrake.Server
{
    using System.Linq;

    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The HandBrake Service
    /// </summary>
    class Program
    {
        /// <summary>
        /// The main.
        /// </summary>
        /// <param name="args">
        /// The args.
        /// </param>
        static void Main(string[] args)
        {
            if (args.Count() != 1)
            {
                IServerService server = new ServerService();
                server.Start("8001");
            }
            else
            {
                IServerService server = new ServerService();
                server.Start(args[0]);
            }
        }
    }
}
