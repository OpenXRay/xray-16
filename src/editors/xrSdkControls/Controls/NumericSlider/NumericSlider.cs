using System;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public sealed partial class NumericSlider : UserControl
    {
        private bool ignoreOnChanged = false;

        public NumericSlider()
        {
            InitializeComponent();
            MinimumSize = Size.Empty; // just to trigger MinimumSize setter logic
        }

        public event EventHandler ValueChanged;

        public decimal Value
        {
            get { return numSpinner.Value; }
            set
            {
                numSpinner.Value = value;
                SliderPos = numSpinner.Value;
            }
        }

        public decimal Minimum
        {
            get { return numSpinner.Minimum; }
            set
            {
                numSpinner.Minimum = value;
                SliderPos = numSpinner.Value;
            }
        }

        public decimal Maximum
        {
            get { return numSpinner.Maximum; }
            set
            {
                numSpinner.Maximum = value;
                SliderPos = numSpinner.Value;
            }
        }

        public bool Hexadecimal
        {
            get { return numSpinner.Hexadecimal; }
            set { numSpinner.Hexadecimal = value; }
        }

        public HorizontalAlignment TextAlign
        {
            get { return numSpinner.TextAlign; }
            set { numSpinner.TextAlign = value; }
        }

        public int DecimalPlaces
        {
            get { return numSpinner.DecimalPlaces; }
            set { numSpinner.DecimalPlaces = value; }
        }

        public int SliderPrecision
        {
            get { return trackBar.Maximum; }
            set { trackBar.Maximum = value; }
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

        private decimal SliderPos
        {
            get
            {
                var norm = trackBar.Value*1.0M/SliderPrecision;
                return Minimum + norm*(Maximum-Minimum);
            }
            set
            {
                try
                {
                    ignoreOnChanged = true;
                    var norm = (value-Minimum)/(Maximum-Minimum);
                    trackBar.Value = (int)(norm*SliderPrecision);
                }
                finally
                {
                    ignoreOnChanged = false;
                }
            }
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            numSpinner.ValueChanged += (obj, args) =>
            {
                if (ignoreOnChanged || SliderPos == numSpinner.Value)
                    return;
                SliderPos = numSpinner.Value;
                OnValueChanged();
            };
            trackBar.ValueChanged += (obj, args) =>
            {
                if (ignoreOnChanged || numSpinner.Value == SliderPos)
                    return;
                ignoreOnChanged = true;
                numSpinner.Value = SliderPos;
                ignoreOnChanged = false;
                OnValueChanged();
            };
        }
    }
}
