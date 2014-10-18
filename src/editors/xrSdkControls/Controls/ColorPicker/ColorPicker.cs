using System;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    // XXX nitrocaster: implement hex color TextBox (NumericBox)
    public sealed partial class ColorPicker : UserControl
    {
        public delegate void ColorChangedEventHandler(object sender, Color color);

        private Color color;
        private bool hexademical;
        
        public ColorPicker()
        {
            InitializeComponent();
        }

        public event ColorChangedEventHandler ColorChanged;

        public Color Value
        {
            get { return color; }
            set
            {
                color = value;
                UpdateColor();
            }
        }

        public byte Red { get; private set; }
        public byte Green { get; private set; }
        public byte Blue { get; private set; }
        public byte Alpha { get; private set; }

        public bool Hexademical
        {
            get { return hexademical; }
            set
            {
                if (hexademical == value)
                    return;
                hexademical = value;
                chkHexademical.Checked = value;
                numRed.Hexadecimal = value;
                numGreen.Hexadecimal = value;
                numBlue.Hexadecimal = value;
                numAlpha.Hexadecimal = value;
            }
        }

        private void ColorPicker_Load(object sender, EventArgs e)
        {
            numRed.ValueChanged += (obj, args) => SyncValues(tbrRed, numRed);
            numGreen.ValueChanged += (obj, args) => SyncValues(tbrGreen, numGreen);
            numBlue.ValueChanged += (obj, args) => SyncValues(tbrBlue, numBlue);
            numAlpha.ValueChanged += (obj, args) => SyncValues(tbrAlpha, numAlpha);
            tbrRed.ValueChanged += (obj, args) => SyncValues(numRed, tbrRed);
            tbrGreen.ValueChanged += (obj, args) => SyncValues(numGreen, tbrGreen);
            tbrBlue.ValueChanged += (obj, args) => SyncValues(numBlue, tbrBlue);
            tbrAlpha.ValueChanged += (obj, args) => SyncValues(numAlpha, tbrAlpha);
            chkHexademical.CheckedChanged += (obj, args) => Hexademical = chkHexademical.Checked;
            UpdateColor();
        }

        private void SyncValues(TrackBar tbr, IntegerUpDown num)
        {
            if (tbr.Value == num.Value)
                return;
            tbr.Value = num.Value;
            UpdateColor();
        }

        private void SyncValues(IntegerUpDown num, TrackBar tbr)
        {
            if (num.Value == tbr.Value)
                return;
            num.Value = tbr.Value;
            UpdateColor();
        }

        private void UpdateColor()
        {
            var newColor = Color.FromArgb(tbrAlpha.Value, tbrRed.Value, tbrGreen.Value, tbrBlue.Value);
            if (pbColor.ColorSample == newColor)
                return;
            pbColor.ColorSample = newColor;
            if (ColorChanged != null)
                ColorChanged(this, Value);
        }
    }
}
