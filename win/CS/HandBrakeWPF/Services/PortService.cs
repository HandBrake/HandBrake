// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PortService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PortService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Net;
    using System.Net.NetworkInformation;

    using HandBrakeWPF.Services.Interfaces;

    public class PortService : IPortService
    {
        private readonly object lockObject = new object();
        private List<int> usedPorts = new List<int>();

        public PortService()
        {
        }

        public void FreePort(int port)
        {
            lock (this.lockObject)
            {
                this.usedPorts.Remove(port);
            }
        }

        public int GetOpenPort(int startPort)
        {
            lock (this.lockObject)
            {
                int foundPort = FindUnusedPort(startPort);

                // If we find a port that's free on the system, but we've already allocated, try find another.
                while (this.usedPorts.Contains(foundPort))
                {
                    int nextPort = foundPort + 1;
                    foundPort = FindUnusedPort(nextPort);
                }

                // Record we've used this port.
                this.usedPorts.Add(foundPort);

                return foundPort;
            }
        }

        private int FindUnusedPort(int startPort)
        {
            if (startPort == 0)
            {
                startPort = 8037;
            }

            int portStartIndex = startPort;

            IPGlobalProperties properties = IPGlobalProperties.GetIPGlobalProperties();
            IPEndPoint[] tcpEndPoints = properties.GetActiveTcpListeners();

            List<int> usedPorts = tcpEndPoints.Select(p => p.Port).ToList<int>();
            int unusedPort = 0;

            unusedPort = Enumerable.Range(portStartIndex, 99).FirstOrDefault(p => !usedPorts.Contains(p));

            return unusedPort;
        }
    }
}
