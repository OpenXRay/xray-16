using System;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public sealed partial class IntegerSlider : UserControl
    {
        public IntegerSlider()
        {
            InitializeComponent();
            MinimumSize = Size.Empty; // just to trigger MinimumSize setter logic
        }

        public event EventHandler ValueChanged;

        public int Value
        {
            get { return numSpinner.Value; }
            set
            {
                numSpinner.Value = value;
                trackBar.Value = value;
            }
        }

        public int Minimum
        {
            get { return numSpinner.Minimum; }
            set
            {
                numSpinner.Minimum = value;
                trackBar.Minimum = value;
            }
        }

        public int Maximum
        {
            get { return numSpinner.Maximum; }
            set
            {
                numSpinner.Maximum = value;
                trackBar.Maximum = value;
            }
        }

        public bool Hexadecimal
        {
            get { return numSpinner.Hexadecimal; }
            set { numSpinner.Hexadecimal = value; }
        }

        /// <summary>
        /// Gets or sets a value to be added to or substracted from the Value property
        /// when the scroll box is moved a small distance.
        /// </summary>
        public int SmallChange
        {
            get { return trackBar.SmallChange; }
            set { trackBar.SmallChange = value; }
        }

        /// <summary>
        /// Gets or sets a value to be added to or substracted from the Value property
        /// when the scroll box is moved a large distance.
        /// </summary>
        public int LargeChange
        {
            get { return trackBar.LargeChange; }
            set { trackBar.LargeChange = value; }
        }

        /// <summary>
        /// Gets or sets a value that specifies the delta between ticks drawn on the control.
        /// </summary>
        public int TickFrequency
        {
            get { return trackBar.TickFrequency; }
            set { trackBar.TickFrequency = value; }
        }

        /// <summary>
        /// Gets or sets a value indicating how to display the tick marks on the track bar.
        /// </summary>
        public TickStyle TickStyle
        {
            get { return trackBar.TickStyle; }
            set { trackBar.TickStyle = value; }
        }

        public int SpinnerWidth
        {
            get { return numSpinner.Width; }
            set
            {
                var minSpinnerWidth = numSpinner.MinimumSize.Width;
                var minTrackBarWidth = trackBar.MinimumSize.Width;
                if (value < minSpinnerWidth)
                    value = minSpinnerWidth;
                if (trackBar.Width+numSpinner.Width-value < minTrackBarWidth)
                {
                    value = trackBar.Width+numSpinner.Width-minTrackBarWidth;
                    if (value == numSpinner.Width)
                        return;
                }
                var delta = value-numSpinner.Width;
                if (delta == 0)
                    return;
                var spinnerLoc = numSpinner.Location;
                numSpinner.Location = new Point(spinnerLoc.X-delta, spinnerLoc.Y);
                trackBar.Width -= delta;
                numSpinner.Width = value;
            }
        }

        public override Size MinimumSize
        {
            get { return base.MinimumSize; }
            set
            {
                var minWidth = numSpinner.MinimumSize.Width+trackBar.MinimumSize.Width;
                if (value.Width < minWidth)
                    value.Width = minWidth;
                base.MinimumSize = value;
            }
        }

        private void OnValueChanged()
        {
            ValueChanged?.Invoke(this, null);
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            numSpinner.ValueChanged += (obj, args) =>
            {
                if (trackBar.Value == numSpinner.Value)
                    return;
                trackBar.Value = numSpinner.Value;
                OnValueChanged();
            };
            trackBar.ValueChanged += (obj, args) =>
            {
                if (numSpinner.Value == trackBar.Value)
                    return;
                numSpinner.Value = trackBar.Value;
                OnValueChanged();
            };
        }
    }
}
