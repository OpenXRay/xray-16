using System;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    /// <summary>
    /// Comprises the information specifying how acceleration should be performed
    /// on a Windows up-down control when the up/down button is pressed for certain
    /// amount of time.
    /// </summary>
    public class IntegerUpDownAcceleration
    {
        private Int32 seconds;    // Ideally we would use UInt32 but it is not CLS-compliant. 
        private Int32 increment;  // Ideally we would use UInt32 but it is not CLS-compliant. 

        public IntegerUpDownAcceleration(Int32 seconds, Int32 increment)
        {
            if (seconds < 0)
            {
                throw new ArgumentOutOfRangeException("seconds < 0");
            }

            if (increment < 0)
            {
                throw new ArgumentOutOfRangeException("increment < 0");
            }

            this.seconds = seconds;
            this.increment = increment;
        }

        /// <summary>
        /// Determines the amount of time for the UpDown control to wait to set the increment 
        /// step when holding the up/down button.
        /// </summary>
        public Int32 Seconds
        {
            get
            {
                return seconds;
            }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("seconds < 0");
                }
                seconds = value;
            }
        }

        /// <summary>
        /// Determines the amount to increment by.
        /// </summary>
        public Int32 Increment
        {
            get
            {
                return increment;
            }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("increment < 0");
                }
                increment = value;
            }
        }
    }
}