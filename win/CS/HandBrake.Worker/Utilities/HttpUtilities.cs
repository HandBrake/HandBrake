// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HttpUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HttpUtilities type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Utilities
{
    using System.IO;
    using System.Net;

    public class HttpUtilities
    {
        public static string GetRequestPostData(HttpListenerRequest request)
        {
            if (!request.HasEntityBody)
            {
                return null;
            }

            using (Stream inputStream = request.InputStream)
            {
                using (StreamReader streamReader = new StreamReader(inputStream, request.ContentEncoding))
                {
                    return streamReader.ReadToEnd();
                }
            }
        }
    }
}
