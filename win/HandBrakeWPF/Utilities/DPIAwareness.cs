// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DPIAwareness.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DPIAwareness type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Windows;
    using System.Windows.Media;

    /// <summary>
    /// The dpi awareness.
    /// </summary>
    public class DPIAwareness
    {
        /// <summary>
        /// The dx.
        /// </summary>
        private static double dx;

        /// <summary>
        /// The dy.
        /// </summary>
        private static double dy;

        /// <summary>
        /// Initializes a new instance of the <see cref="DPIAwareness"/> class.
        /// </summary>
        public DPIAwareness()
        {
            var presentationSource = PresentationSource.FromVisual(Application.Current.MainWindow);
            if (presentationSource != null && presentationSource.CompositionTarget != null)
            {
                Matrix m = presentationSource.CompositionTarget.TransformToDevice;
                dx = m.M11;
                dy = m.M22;
            }
        }

        /// <summary>
        /// Gets the dpix.
        /// </summary>
        public double Dpix
        {
            get
            {
                return dx;
            }
        }

        /// <summary>
        /// Gets the dpi y.
        /// </summary>
        public double DpiY
        {
            get
            {
                return dy;
            }
        }
    }
}
